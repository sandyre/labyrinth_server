//
//  masterserver.cpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#include "masterserver.hpp"

#include <chrono>
#include <limits>
#include <memory>
#include <utility>
#include <Poco/Net/MailMessage.h>
#include <Poco/Net/SMTPClientSession.h>
#include <Poco/Data/SQLite/Connector.h>
#include "msnet_generated.h"

using namespace MasterEvent;
using namespace Poco::Data::Keywords;
using namespace std::chrono_literals;
using Poco::Data::Session;
using Poco::Data::Statement;

MasterServer::MasterServer() :
m_nSystemStatus(0),
m_oGenerator(228), // FIXME: random should always be random!
m_oDistr(std::uniform_int_distribution<>(std::numeric_limits<int32_t>::min(),
                                         std::numeric_limits<int32_t>::max())),
m_bCommutationNeeded(false)
{
}

MasterServer::~MasterServer()
{
    m_oLogSys.Close();
    m_oSocket.close();
    m_oMsgBuilder << "Shutdown";
    m_oLogSys.Write(m_oMsgBuilder.str());
    m_oMsgBuilder.str("");
}

void
MasterServer::init(uint32_t Port)
{
        // Init logging
    m_oLogSys.Init("MS", LogSystem::Mode::STDIO);
    m_nSystemStatus |= SystemStatus::LOG_SYSTEM_ACTIVE;
    
    m_oMsgBuilder << "Started at " << Port;
    m_oLogSys.Write(m_oMsgBuilder.str());
    m_oMsgBuilder.str("");
    
    m_oMsgBuilder << "Initializing network...";
    m_oLogSys.Write(m_oMsgBuilder.str());
    m_oMsgBuilder.str("");
    
        // Init Net
    try
    {
        Poco::Net::SocketAddress sock_addr(Poco::Net::IPAddress(), Port);
        m_oSocket.bind(sock_addr);
        
        m_nSystemStatus |= SystemStatus::NETWORK_SYSTEM_ACTIVE;
        m_oMsgBuilder << "DONE";
        m_oLogSys.Write(m_oMsgBuilder.str());
        m_oMsgBuilder.str("");
    }
    catch(std::exception& e)
    {
        m_oMsgBuilder << "FAILED";
        m_oLogSys.Write(m_oMsgBuilder.str());
        m_oMsgBuilder.str("");
    }
    
    for(uint32_t i = 1931; i < (1931 + 2000); ++i)
    {
        Poco::Net::DatagramSocket socket;
        Poco::Net::SocketAddress addr(Poco::Net::IPAddress(), i);
        
        try
        {
            socket.bind(addr);
            m_qAvailablePorts.push(i);
            socket.close();
        } catch (std::exception e)
        {
                // port is busy
        }
    }
    
    m_oMsgBuilder << "Initializing mail service...";
    m_oLogSys.Write(m_oMsgBuilder.str());
    m_oMsgBuilder.str("");
    
        // Init mail
    try
    {
        m_poMailClient = std::make_unique<Poco::Net::SMTPClientSession>("smtp.timeweb.ru",
                                                                        25);
        
        m_poMailClient->login(Poco::Net::SMTPClientSession::LoginMethod::AUTH_LOGIN,
                              "noreply-labyrinth@hate-red.com",
                              "VdAysb4A");
        
        m_nSystemStatus |= SystemStatus::EMAIL_SYSTEM_ACTIVE;
        m_oMsgBuilder << "DONE";
        m_oLogSys.Write(m_oMsgBuilder.str());
        m_oMsgBuilder.str("");
    }
    catch(std::exception& e)
    {
        m_oMsgBuilder << "FAILED";
        m_oLogSys.Write(m_oMsgBuilder.str());
        m_oMsgBuilder.str("");
    }
    
    
    m_oMsgBuilder << "Initializing database service...";
    m_oLogSys.Write(m_oMsgBuilder.str());
    m_oMsgBuilder.str("");
    
        // Init DB
    try
    {
        Poco::Data::SQLite::Connector::registerConnector(),
        m_poDBSession = std::make_unique<Poco::Data::Session>("SQLite",
                                                              "labyrinth.db");
        
        m_nSystemStatus |= SystemStatus::EMAIL_SYSTEM_ACTIVE;
        m_oMsgBuilder << "DONE";
        m_oLogSys.Write(m_oMsgBuilder.str());
        m_oMsgBuilder.str("");
    }
    catch(std::exception& e)
    {
        m_oMsgBuilder << "FAILED";
        m_oLogSys.Write(m_oMsgBuilder.str());
        m_oMsgBuilder.str("");
    }
    
    if(!(SystemStatus::NETWORK_SYSTEM_ACTIVE & m_nSystemStatus) &&
       !(SystemStatus::DATABASE_SYSTEM_ACTIVE & m_nSystemStatus))
    {
        m_oMsgBuilder << "Critical systems failed to start, aborting";
        m_oLogSys.Write(m_oMsgBuilder.str());
        m_oMsgBuilder.str("");
        
        exit(1);
    }
    
        // Everything done
    m_oMsgBuilder << "MasterServer started running";
    m_oLogSys.Write(m_oMsgBuilder.str());
    m_oMsgBuilder.str("");
}

void
MasterServer::run()
{
    auto tick_start = std::chrono::steady_clock::now();
    auto tick_end = std::chrono::steady_clock::now();
    auto tick_duration = std::chrono::duration_cast<std::chrono::microseconds>(tick_end - tick_start);
    
    while(true)
    {
        tick_start = std::chrono::steady_clock::now();
        while(m_oSocket.available())
        {
            ProcessIncomingMessage();
            if(m_bCommutationNeeded)
            {
                CommutatePlayers();
            }
        }
        std::this_thread::sleep_for(1ms);
        
        if(m_usFreementTimer > 30s)
        {
            m_usFreementTimer = 0us;
            FreeResourcesAndSaveResults();
        }
        
        tick_end = std::chrono::steady_clock::now();
        tick_duration = std::chrono::duration_cast<std::chrono::microseconds>(tick_end - tick_start);
        
        m_usFreementTimer += tick_duration;
    }
}

void
MasterServer::ProcessIncomingMessage()
{
    Poco::Net::SocketAddress sender_addr;
    
    auto size = m_oSocket.receiveFrom(m_aDataBuffer.data(), 256, sender_addr);
    auto msg = GetMessage(m_aDataBuffer.data());
    
    switch(msg->message_type())
    {
        case Messages_CLPing:
        {
            auto pong = CreateSVPing(m_oFlatBuilder);
            auto smsg = CreateMessage(m_oFlatBuilder,
                                      Messages_CLPing,
                                      pong.Union());
            m_oFlatBuilder.Finish(smsg);
            
            m_oSocket.sendTo(m_oFlatBuilder.GetBufferPointer(),
                             m_oFlatBuilder.GetSize(),
                             sender_addr);
            m_oFlatBuilder.Clear();
            break;
        }
            
        case Messages_CLRegister:
        {
            auto registr = static_cast<const CLRegister*>(msg->message());
            size_t mail_presented = 0;
            std::string email(registr->email()->c_str());
            std::string password(registr->password()->c_str());
            
            std::hash<std::string> hash_fn;
            size_t email_hash = hash_fn(email);
            
                // Check that player isnt registered already
            Poco::Data::Statement select(*m_poDBSession);
            select << "SELECT COUNT(*) FROM players WHERE email=?", into(mail_presented), use(email), now;
            
                // New user, add him
            if(mail_presented == 0)
            {
                    // Add to DB
                Poco::Data::Statement insert(*m_poDBSession);
                insert << "INSERT INTO PLAYERS(uid, email, password) VALUES(?, ?, ?)", use(email_hash), use(email), use(password);
                insert.execute();
                
                    // Send email notification
                if(m_nSystemStatus & SystemStatus::EMAIL_SYSTEM_ACTIVE)
                {
                    Poco::Net::MailMessage notification;
                    notification.addRecipient(Poco::Net::MailRecipient(Poco::Net::MailRecipient::PRIMARY_RECIPIENT,
                                                                       email,
                                                                       "user"));
                    notification.setSubject("Registration completed");
                    
                    std::ostringstream mail;
                    mail << "Thanks for your registration!\n";
                    mail << "Your email: " << email << "\n";
                    mail << "Your password: " << password << "\n";
                    mail << "Best regards, \nhate-red";
                    notification.setContent(mail.str());
                    m_poMailClient->sendMessage(notification);
                }
                
                    // Send response to client
                auto response = CreateSVRegister(m_oFlatBuilder,
                                                 RegistrationStatus_SUCCESS);
                auto msg = CreateMessage(m_oFlatBuilder,
                                         Messages_SVRegister,
                                         response.Union());
                m_oFlatBuilder.Finish(msg);
                
                m_oSocket.sendTo(m_oFlatBuilder.GetBufferPointer(),
                                 m_oFlatBuilder.GetSize(),
                                 sender_addr);
                m_oFlatBuilder.Clear();
            }
            else // email already taken
            {
                    // Send response to client
                auto response = CreateSVRegister(m_oFlatBuilder,
                                                 RegistrationStatus_EMAIL_TAKEN);
                auto msg = CreateMessage(m_oFlatBuilder,
                                         Messages_SVRegister,
                                         response.Union());
                m_oFlatBuilder.Finish(msg);
                
                m_oSocket.sendTo(m_oFlatBuilder.GetBufferPointer(),
                                 m_oFlatBuilder.GetSize(),
                                 sender_addr);
                m_oFlatBuilder.Clear();
            }
            
            break;
        }
            
        case Messages_CLLogin:
        {
            auto registr = static_cast<const CLLogin*>(msg->message());
            
            Poco::Nullable<std::string> stored_pass;
            std::string email(registr->email()->c_str());
            std::string password(registr->password()->c_str());
            
                // Check that player isnt registered already
            Poco::Data::Statement select(*m_poDBSession);
            select << "SELECT PASSWORD FROM PLAYERS WHERE email=?", into(stored_pass), use(email), now;
            
            if(!stored_pass.isNull())
            {
                    // password is correct, notify player
                if(stored_pass == password)
                {
                        // Send response to client
                    auto response = CreateSVLogin(m_oFlatBuilder,
                                                  LoginStatus_SUCCESS);
                    auto msg = CreateMessage(m_oFlatBuilder,
                                             Messages_SVLogin,
                                             response.Union());
                    m_oFlatBuilder.Finish(msg);
                    
                    m_oSocket.sendTo(m_oFlatBuilder.GetBufferPointer(),
                                     m_oFlatBuilder.GetSize(),
                                     sender_addr);
                    m_oFlatBuilder.Clear();
                }
                else // password is wrong
                {
                        // Send response to client
                    auto response = CreateSVLogin(m_oFlatBuilder,
                                                  LoginStatus_WRONG_INPUT);
                    auto msg = CreateMessage(m_oFlatBuilder,
                                             Messages_SVLogin,
                                             response.Union());
                    m_oFlatBuilder.Finish(msg);
                    
                    m_oSocket.sendTo(m_oFlatBuilder.GetBufferPointer(),
                                     m_oFlatBuilder.GetSize(),
                                     sender_addr);
                    m_oFlatBuilder.Clear();
                }
            }
            else // player is not registered, or wrong password
            {
                    // Send response to client
                
                    // FIXME: when debug is done, set to WRONG_INPUT
                auto response = CreateSVLogin(m_oFlatBuilder,
                                              LoginStatus_WRONG_INPUT);
                auto msg = CreateMessage(m_oFlatBuilder,
                                         Messages_SVLogin,
                                         response.Union());
                m_oFlatBuilder.Finish(msg);
                
                m_oSocket.sendTo(m_oFlatBuilder.GetBufferPointer(),
                                 m_oFlatBuilder.GetSize(),
                                 sender_addr);
                m_oFlatBuilder.Clear();
            }
            
            break;
        }
            
        case Messages_CLFindGame:
        {
            auto finder = static_cast<const CLFindGame*>(msg->message());
            
            if(finder->cl_version_major() == GAMEVERSION_MAJOR)
            {
                auto gs_accepted = CreateSVFindGame(m_oFlatBuilder,
                                                    finder->player_uid(),
                                                    ConnectionResponse_ACCEPTED);
                auto gs_event = CreateMessage(m_oFlatBuilder,
                                              Messages_SVFindGame,
                                              gs_accepted.Union());
                m_oFlatBuilder.Finish(gs_event);
                m_oSocket.sendTo(m_oFlatBuilder.GetBufferPointer(),
                                 m_oFlatBuilder.GetSize(),
                                 sender_addr);
                m_oFlatBuilder.Clear();
            }
            else
            {
                auto gs_refused = CreateSVFindGame(m_oFlatBuilder,
                                                   finder->player_uid(),
                                                   ConnectionResponse_REFUSED);
                auto gs_event = CreateMessage(m_oFlatBuilder,
                                              Messages_SVFindGame,
                                              gs_refused.Union());
                m_oFlatBuilder.Finish(gs_event);
                m_oSocket.sendTo(m_oFlatBuilder.GetBufferPointer(),
                                 m_oFlatBuilder.GetSize(),
                                 sender_addr);
                m_oFlatBuilder.Clear();
                break;
            }
                // check that player isnt already in pool
            auto iter = std::find_if(m_aPlayersPool.begin(),
                                     m_aPlayersPool.end(),
                                     [finder](Player& player)
                                     {
                                         return player.GetUID() == finder->player_uid();
                                     });
            
                // player is not in pool already, add him
            if(iter == m_aPlayersPool.end())
            {
                Player player(finder->player_uid(),
                              sender_addr,
                              std::string(""));
                
                m_oMsgBuilder << "Player connected, UID: " << finder->player_uid();
                m_oLogSys.Write(m_oMsgBuilder.str());
                m_oMsgBuilder.str("");
                
                m_aPlayersPool.emplace_back(std::move(player));
            }
            m_bCommutationNeeded = true; // player asked game, need to recompute commutation
            break;
        }
            
        default:
            m_oMsgBuilder << "Undefined packet received";
            m_oLogSys.Write(m_oMsgBuilder.str());
            m_oMsgBuilder.str("");
            break;
    }
}

void
MasterServer::FreeResourcesAndSaveResults()
{
    m_oMsgBuilder << "Started gameservers freement... ";
    
        // Free resources of finished servers
    auto servers_freed = 0;
    m_aGameServers.erase(
                         std::remove_if(m_aGameServers.begin(),
                                        m_aGameServers.end(),
                                        [this, &servers_freed](std::unique_ptr<GameServer> const& server)
                                        {
                                            if(server->GetState() == GameServer::State::FINISHED)
                                            {
                                                ++servers_freed;
                                                m_qAvailablePorts.push(server->GetConfig().nPort);
                                                return true;
                                            }
                                            return false;
                                        }),
                         m_aGameServers.end()
                         );
    
    m_oMsgBuilder << servers_freed << " servers freed";
    m_oLogSys.Write(m_oMsgBuilder.str());
    m_oMsgBuilder.str("");
}

void
MasterServer::CommutatePlayers()
{
        // if there are some players, check that there is a server for them
    if(m_aPlayersPool.size() != 0)
    {
        auto servers_available = std::count_if(m_aGameServers.begin(),
                                               m_aGameServers.end(),
                                               [this](std::unique_ptr<GameServer> const& gs)
                                               {
                                                   return gs->GetState() == GameServer::State::LOBBY_FORMING;
                                               });
        
            // no servers? start a new
        if(servers_available == 0)
        {
            uint32_t nGSPort = m_qAvailablePorts.front();
            m_qAvailablePorts.pop();
            
            GameServer::Configuration config;
            config.nPlayers = 1; // +-
            config.nRandomSeed = m_oDistr(m_oGenerator);
            config.nPort = nGSPort;
            
            m_aGameServers.push_back(std::make_unique<GameServer>(config));
        }
    }
    
        // FIXME: on a big amount of gameservers, may not run correctly
    for(auto& gs : m_aGameServers)
    {
        if(m_aPlayersPool.size() == 0) // optimization (no need to loop more through gss)
            break;
        
        if(gs->GetState() == GameServer::State::LOBBY_FORMING)
        {
            auto serv_config = gs->GetConfig();
                // transfer player to GS
            auto game_found = CreateSVGameFound(m_oFlatBuilder,
                                                serv_config.nPort);
            auto ms_event = CreateMessage(m_oFlatBuilder,
                                          Messages_SVGameFound,
                                          game_found.Union());
            m_oFlatBuilder.Finish(ms_event);
            
            auto& player = m_aPlayersPool.front();
            
            m_oSocket.sendTo(m_oFlatBuilder.GetBufferPointer(),
                             m_oFlatBuilder.GetSize(),
                             player.GetAddress());
            
            m_oFlatBuilder.Clear();
            m_aPlayersPool.pop_front(); // remove a player from queue
        }
    }
    
    m_bCommutationNeeded = false;
}
