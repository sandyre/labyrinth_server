//
//  masterserver.cpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#include "masterserver.hpp"
#include <iostream>
#include <limits>
#include <memory>
#include <regex>
#include <sstream>
#include <Poco/Net/MailMessage.h>
#include <Poco/Net/SMTPClientSession.h>
#include <Poco/Data/SQLite/Connector.h>
#include "msnet_generated.h"

using namespace Poco::Data::Keywords;
using Poco::Data::Session;
using Poco::Data::Statement;

MasterServer::MasterServer() :
m_nSystemStatus(0),
m_oGenerator(228), // FIXME: random should always be random!
m_oDistr(std::uniform_int_distribution<>(std::numeric_limits<int32_t>::min(),
                                         std::numeric_limits<int32_t>::max())),
m_oMailClient("smtp.timeweb.ru", 25)
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
        m_oMailClient.login(Poco::Net::SMTPClientSession::LoginMethod::AUTH_LOGIN,
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
        m_pDBSession = std::make_unique<Poco::Data::Session>("SQLite", "labyrinth.db");
        
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
    flatbuffers::FlatBufferBuilder builder; // is needed to build output data
    Poco::Net::SocketAddress sender_addr;
    char request[256];
    
    while(true)
    {
        auto size = m_oSocket.receiveFrom(request, 256, sender_addr);
        
        auto event = MSNet::GetMSEvent(request);
        
        switch(event->event_type())
        {
            case MSNet::MSEvents_CLPing:
            {
                auto pong = MSNet::CreateMSPing(builder);
                builder.Finish(pong);
                m_oSocket.sendTo(builder.GetBufferPointer(),
                                 builder.GetSize(),
                                 sender_addr);
                builder.Clear();
                break;
            }
                
            case MSNet::MSEvents_CLRegister:
            {
                auto registr = static_cast<const MSNet::CLRegister*>(event->event());
                size_t mail_presented = 0;
                std::string email(registr->email()->c_str());
                std::string password(registr->password()->c_str());
                
                    // Check that player isnt registered already
                Poco::Data::Statement select(*m_pDBSession);
                select << "SELECT COUNT(*) FROM players WHERE email=?", into(mail_presented), use(email), now;
                
                    // New user, add him
                if(mail_presented == 0)
                {
                        // Add to DB
                    Poco::Data::Statement insert(*m_pDBSession);
                    insert << "INSERT INTO PLAYERS(email, password) VALUES(?, ?)", use(email), use(password);
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
                        m_oMailClient.sendMessage(notification);
                    }
                    
                        // Send response to client
                    auto response = MSNet::CreateSVRegister(builder,
                                                            MSNet::RegistrationStatus_SUCCESS);
                    auto msg = MSNet::CreateMSEvent(builder,
                                                    MSNet::MSEvents_SVRegister,
                                                    response.Union());
                    builder.Finish(msg);
                    
                    m_oSocket.sendTo(builder.GetBufferPointer(),
                                     builder.GetSize(),
                                     sender_addr);
                    builder.Clear();
                }
                else // email already taken
                {
                        // Send response to client
                    auto response = MSNet::CreateSVRegister(builder,
                                                            MSNet::RegistrationStatus_EMAIL_TAKEN);
                    auto msg = MSNet::CreateMSEvent(builder,
                                                    MSNet::MSEvents_SVRegister,
                                                    response.Union());
                    builder.Finish(msg);
                    
                    m_oSocket.sendTo(builder.GetBufferPointer(),
                                     builder.GetSize(),
                                     sender_addr);
                    builder.Clear();
                }
                
                break;
            }
                
            case MSNet::MSEvents_CLLogin:
            {
                auto registr = static_cast<const MSNet::CLRegister*>(event->event());
                
                Poco::Nullable<std::string> stored_pass;
                std::string email(registr->email()->c_str());
                std::string password(registr->password()->c_str());
                
                    // Check that player isnt registered already
                Poco::Data::Statement select(*m_pDBSession);
                select << "SELECT PASSWORD FROM PLAYERS WHERE email=?", into(stored_pass), use(email), now;
                
                if(!stored_pass.isNull())
                {
                        // password is correct, notify player
                    if(stored_pass == password)
                    {
                            // Send response to client
                        auto response = MSNet::CreateSVLogin(builder,
                                                             MSNet::LoginStatus_SUCCESS);
                        auto msg = MSNet::CreateMSEvent(builder,
                                                        MSNet::MSEvents_SVLogin,
                                                        response.Union());
                        builder.Finish(msg);
                        
                        m_oSocket.sendTo(builder.GetBufferPointer(),
                                         builder.GetSize(),
                                         sender_addr);
                        builder.Clear();
                    }
                    else // password is wrong
                    {
                            // Send response to client
                        auto response = MSNet::CreateSVLogin(builder,
                                                             MSNet::LoginStatus_WRONG_INPUT);
                        auto msg = MSNet::CreateMSEvent(builder,
                                                        MSNet::MSEvents_SVLogin,
                                                        response.Union());
                        builder.Finish(msg);
                        
                        m_oSocket.sendTo(builder.GetBufferPointer(),
                                         builder.GetSize(),
                                         sender_addr);
                        builder.Clear();
                    }
                }
                else // player is not registered, or wrong password
                {
                        // Send response to client
                    
                    // FIXME: when debug is done, set to WRONG_INPUT
                    auto response = MSNet::CreateSVLogin(builder,
                                                         MSNet::LoginStatus_WRONG_INPUT);
                    auto msg = MSNet::CreateMSEvent(builder,
                                                    MSNet::MSEvents_SVLogin,
                                                    response.Union());
                    builder.Finish(msg);
                    
                    m_oSocket.sendTo(builder.GetBufferPointer(),
                                     builder.GetSize(),
                                     sender_addr);
                    builder.Clear();
                }
                
                break;
            }
                
            case MSNet::MSEvents_CLFindGame:
            {
                auto finder = static_cast<const MSNet::CLFindGame*>(event->event());
                
                if(finder->cl_version_major() == GAMEVERSION_MAJOR)
                {
                    auto gs_accepted = MSNet::CreateSVFindGame(builder,
                                                               finder->player_uid(),
                                                               MSNet::ConnectionResponse_ACCEPTED);
                    auto gs_event = MSNet::CreateMSEvent(builder,
                                                         MSNet::MSEvents_SVFindGame,
                                                         gs_accepted.Union());
                    builder.Finish(gs_event);
                    m_oSocket.sendTo(builder.GetBufferPointer(),
                                     builder.GetSize(),
                                     sender_addr);
                    builder.Clear();
                }
                else
                {
                    auto gs_refused = MSNet::CreateSVFindGame(builder,
                                                              finder->player_uid(),
                                                              MSNet::ConnectionResponse_REFUSED);
                    auto gs_event = MSNet::CreateMSEvent(builder,
                                                         MSNet::MSEvents_SVFindGame,
                                                         gs_refused.Union());
                    builder.Finish(gs_event);
                    m_oSocket.sendTo(builder.GetBufferPointer(),
                                     builder.GetSize(),
                                     sender_addr);
                    builder.Clear();
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
                
                break;
            }
            
            default:
                m_oMsgBuilder << "Undefined packet received";
                m_oLogSys.Write(m_oMsgBuilder.str());
                m_oMsgBuilder.str("");
                break;
        }
        
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
                auto game_found = MSNet::CreateMSGameFound(builder, serv_config.nPort);
                auto ms_event = MSNet::CreateMSEvent(builder, MSNet::MSEvents_MSGameFound, game_found.Union());
                builder.Finish(ms_event);
                
                auto& player = m_aPlayersPool.front();

                m_oSocket.sendTo(builder.GetBufferPointer(),
                                 builder.GetSize(),
                                 player.GetAddress());
                
                builder.Clear();
                m_aPlayersPool.pop_front(); // remove a player from queue
            }
        }
        
            // do something with finished servers
        m_aGameServers.erase(
                             std::remove_if(m_aGameServers.begin(),
                                            m_aGameServers.end(),
                                            [this](std::unique_ptr<GameServer> const& server)
                                            {
                                                if(server->GetState() == GameServer::State::FINISHED)
                                                {
                                                    m_qAvailablePorts.push(server->GetConfig().nPort);
                                                    return true;
                                                }
                                                return false;
                                            }),
                             m_aGameServers.end()
                             );
    }
}
