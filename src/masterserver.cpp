//
//  masterserver.cpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#include "masterserver.hpp"

#include "msnet_generated.h"

#include <Poco/Data/MySQL/Connector.h>
#include <Poco/Data/MySQL/MySQLException.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/URI.h>

#include <chrono>
#include <limits>
#include <memory>
#include <utility>

using namespace MasterEvent;
using namespace Poco::Data::Keywords;
using namespace std::chrono_literals;
using Poco::Data::Statement;

MasterServer::MasterServer() : m_nSystemStatus(0), _randGenerator(228), // FIXME: random should always be random!
                               _randDistr(std::uniform_int_distribution<>(std::numeric_limits<int32_t>::min(),
                                                                          std::numeric_limits<int32_t>::max())),
                               _commutationNeeded(false)
{

}

MasterServer::~MasterServer()
{
    _logSystem.Close();
    _socket.close();
    _msgBuilder << "Shutdown";
    _logSystem.Info(_msgBuilder.str());
    _msgBuilder.str("");
}

void MasterServer::init(uint32_t Port)
{
    // Init logging
    _logSystem.Init("MasterServer",
                    LogSystem::Mode::STDIO);
    m_nSystemStatus |= SystemStatus::LOG_SYSTEM_ACTIVE;

    _msgBuilder << "MasterServer booting starts";
    _logSystem.Warning(_msgBuilder.str());
    _msgBuilder.str("");

    _msgBuilder << "Labyrinth core version: ";
    _msgBuilder << GAMEVERSION_MAJOR << "." << GAMEVERSION_MINOR << "." << GAMEVERSION_BUILD;
    _logSystem.Warning(_msgBuilder.str());
    _msgBuilder.str("");

    // Init Net
    _msgBuilder << "Initializing network...";
    _logSystem.Warning(_msgBuilder.str());
    _msgBuilder.str("");

    // Check inet connection
    try
    {
        Poco::Net::HTTPClientSession session("api.ipify.org");
        session.setTimeout(Poco::Timespan(1,
                                          0));
        Poco::Net::HTTPRequest  request(Poco::Net::HTTPRequest::HTTP_GET,
                                        "/");
        Poco::Net::HTTPResponse response;
        session.sendRequest(request);

        std::istream& rs = session.receiveResponse(response);
        if(response.getStatus() == Poco::Net::HTTPResponse::HTTPStatus::HTTP_OK)
        {
            _msgBuilder << "Public IP: " << rs.rdbuf();
            _logSystem.Info(_msgBuilder.str());
            _msgBuilder.str("");

            _msgBuilder << "Binding to port " << Port;
            _logSystem.Info(_msgBuilder.str());
            _msgBuilder.str("");

            Poco::Net::SocketAddress sock_addr(Poco::Net::IPAddress(),
                                               Port);
            _socket.bind(sock_addr);

            _msgBuilder << "OK";
            _logSystem.Info(_msgBuilder.str());
            _msgBuilder.str("");

            m_nSystemStatus |= SystemStatus::NETWORK_SYSTEM_ACTIVE;

            _msgBuilder << "Network initialization done";
            _logSystem.Info(_msgBuilder.str());
            _msgBuilder.str("");
        }
    } catch(std::exception& e)
    {
        _msgBuilder << "FAILED. Reason: " << e.what();
        _logSystem.Error(_msgBuilder.str());
        _msgBuilder.str("");
    }

    // Fill ports pool
    for(uint32_t i = 1931;
        i < (1931 + 150);
        ++i)
    {
        Poco::Net::DatagramSocket socket;
        Poco::Net::SocketAddress addr(Poco::Net::IPAddress(),
                                      i);

        try
        {
            socket.bind(addr);
            _availablePorts.push(i);
            socket.close();
        } catch(std::exception e)
        {
            // port is busy
        }
    }

    // Init gameservers threadpool
    _msgBuilder << "Initializing gameservers threadpool...";
    _logSystem.Warning(_msgBuilder.str());
    _msgBuilder.str("");
    _threadPool = std::make_unique<Poco::ThreadPool>(10, // min
                                                     40, // max
                                                     120, // idle time
                                                     0); // stack size
    _msgBuilder << "DONE";
    _logSystem.Info(_msgBuilder.str());
    _msgBuilder.str("");

    // Init DB
    _msgBuilder << "Initializing database service...";
    _logSystem.Warning(_msgBuilder.str());
    _msgBuilder.str("");

    try
    {
        using namespace Poco::Data;
        MySQL::Connector::registerConnector();
        std::string con_params =
                            "host=127.0.0.1;user=masterserver;db=labyrinth;password=2(3oOS1E;compress=true;auto-reconnect=true";
        _dbSession = std::make_unique<Session>("MySQL",
                                               con_params);

        m_nSystemStatus |= SystemStatus::DATABASE_SYSTEM_ACTIVE;
        _msgBuilder << "DONE";
        _logSystem.Info(_msgBuilder.str());
        _msgBuilder.str("");
    } catch(std::exception& e)
    {
        _msgBuilder << "FAILED";
        _logSystem.Error(_msgBuilder.str());
        _msgBuilder.str("");
    }

    if(!(SystemStatus::NETWORK_SYSTEM_ACTIVE & m_nSystemStatus) ||
       !(SystemStatus::DATABASE_SYSTEM_ACTIVE & m_nSystemStatus))
    {
        _msgBuilder << "Critical systems failed to start, aborting";
        _logSystem.Error(_msgBuilder.str());
        _msgBuilder.str("");

        exit(1);
    }

    // Everything done
    _msgBuilder << "MasterServer is running";
    _logSystem.Warning(_msgBuilder.str());
    _msgBuilder.str("");

    // Advanced init - freement timer
    _freementTimer = std::make_unique<Poco::Timer>(0,
                                                   30000);
    Poco::TimerCallback<MasterServer> free_callback(* this,
                                                    & MasterServer::FreeResourcesAndSaveResults);
    _freementTimer->start(free_callback);
}

void MasterServer::run()
{
    this->init(1930);
    this->service_loop();
}

void MasterServer::service_loop()
{
    while(true)
    {
        ProcessIncomingMessage();
        if(_commutationNeeded)
            CommutatePlayers();
    }
}

void MasterServer::ProcessIncomingMessage()
{
    Poco::Net::SocketAddress sender_addr;

    auto size = _socket.receiveFrom(_dataBuffer.data(),
                                    256,
                                    sender_addr);
    auto msg  = GetMessage(_dataBuffer.data());

    switch(msg->message_type())
    {
        case Messages_CLPing:
        {
            auto pong = CreateSVPing(_flatBuilder);
            auto smsg = CreateMessage(_flatBuilder,
                                      Messages_CLPing,
                                      pong.Union());
            _flatBuilder.Finish(smsg);

            _socket.sendTo(_flatBuilder.GetBufferPointer(),
                           _flatBuilder.GetSize(),
                           sender_addr);
            _flatBuilder.Clear();
            break;
        }

        case Messages_CLRegister:
        {
            auto        registr        = static_cast<const CLRegister *>(msg->message());
            size_t      mail_presented = 0;
            std::string email(registr->email()->c_str());
            std::string password(registr->password()->c_str());

            // Check that player isnt registered already
            Poco::Data::Statement select(* _dbSession);
            select << "SELECT COUNT(*) FROM players WHERE email=?", into(mail_presented), use(email), now;

            // New user, add him
            if(mail_presented == 0)
            {
                // Add to DB
                Poco::Data::Statement insert(* _dbSession);
                insert << "INSERT INTO players(email, password) VALUES(?, ?)", use(email), use(password), now;

                // Send response to client
                auto response = CreateSVRegister(_flatBuilder,
                                                 RegistrationStatus_SUCCESS);
                auto msg      = CreateMessage(_flatBuilder,
                                              Messages_SVRegister,
                                              response.Union());
                _flatBuilder.Finish(msg);

                _socket.sendTo(_flatBuilder.GetBufferPointer(),
                               _flatBuilder.GetSize(),
                               sender_addr);
                _flatBuilder.Clear();
            }
            else // email already taken
            {
                // Send response to client
                auto response = CreateSVRegister(_flatBuilder,
                                                 RegistrationStatus_EMAIL_TAKEN);
                auto msg      = CreateMessage(_flatBuilder,
                                              Messages_SVRegister,
                                              response.Union());
                _flatBuilder.Finish(msg);

                _socket.sendTo(_flatBuilder.GetBufferPointer(),
                               _flatBuilder.GetSize(),
                               sender_addr);
                _flatBuilder.Clear();
            }

            break;
        }

        case Messages_CLLogin:
        {
            auto registr = static_cast<const CLLogin *>(msg->message());

            Poco::Nullable<std::string> stored_pass;
            std::string                 email(registr->email()->c_str());
            std::string                 password(registr->password()->c_str());

            // Check that player isnt registered already
            Poco::Data::Statement select(* _dbSession);
            select << "SELECT PASSWORD FROM labyrinth.players WHERE email=?", into(stored_pass), use(email), now;

            if(!stored_pass.isNull())
            {
                // password is correct, notify player
                if(stored_pass == password)
                {
                    // Send response to client
                    auto response = CreateSVLogin(_flatBuilder,
                                                  LoginStatus_SUCCESS);
                    auto msg      = CreateMessage(_flatBuilder,
                                                  Messages_SVLogin,
                                                  response.Union());
                    _flatBuilder.Finish(msg);

                    _socket.sendTo(_flatBuilder.GetBufferPointer(),
                                   _flatBuilder.GetSize(),
                                   sender_addr);
                    _flatBuilder.Clear();
                } else // password is wrong
                {
                    // Send response to client
                    auto response = CreateSVLogin(_flatBuilder,
                                                  LoginStatus_WRONG_INPUT);
                    auto msg      = CreateMessage(_flatBuilder,
                                                  Messages_SVLogin,
                                                  response.Union());
                    _flatBuilder.Finish(msg);

                    _socket.sendTo(_flatBuilder.GetBufferPointer(),
                                   _flatBuilder.GetSize(),
                                   sender_addr);
                    _flatBuilder.Clear();
                }
            } else // player is not registered, or wrong password
            {
                // Send response to client

                // FIXME: when debug is done, set to WRONG_INPUT
                auto response = CreateSVLogin(_flatBuilder,
                                              LoginStatus_WRONG_INPUT);
                auto msg      = CreateMessage(_flatBuilder,
                                              Messages_SVLogin,
                                              response.Union());
                _flatBuilder.Finish(msg);

                _socket.sendTo(_flatBuilder.GetBufferPointer(),
                               _flatBuilder.GetSize(),
                               sender_addr);
                _flatBuilder.Clear();
            }

            break;
        }

        case Messages_CLFindGame:
        {
            auto finder = static_cast<const CLFindGame *>(msg->message());

            if(finder->cl_version_major() == GAMEVERSION_MAJOR)
            {
                auto gs_accepted = CreateSVFindGame(_flatBuilder,
                                                    finder->player_uid(),
                                                    ConnectionResponse_ACCEPTED);
                auto gs_event    = CreateMessage(_flatBuilder,
                                                 Messages_SVFindGame,
                                                 gs_accepted.Union());
                _flatBuilder.Finish(gs_event);
                _socket.sendTo(_flatBuilder.GetBufferPointer(),
                               _flatBuilder.GetSize(),
                               sender_addr);
                _flatBuilder.Clear();
            } else
            {
                auto gs_refused = CreateSVFindGame(_flatBuilder,
                                                   finder->player_uid(),
                                                   ConnectionResponse_REFUSED);
                auto gs_event   = CreateMessage(_flatBuilder,
                                                Messages_SVFindGame,
                                                gs_refused.Union());
                _flatBuilder.Finish(gs_event);
                _socket.sendTo(_flatBuilder.GetBufferPointer(),
                               _flatBuilder.GetSize(),
                               sender_addr);
                _flatBuilder.Clear();
                break;
            }
            // check that player isnt already in pool
            auto iter   = std::find_if(_playersPool.begin(),
                                       _playersPool.end(),
                                       [finder](Player& player)
                                       {
                                           return player.GetUID() == finder->player_uid();
                                       });

            // player is not in pool already, add him
            if(iter == _playersPool.end())
            {
                Player player(finder->player_uid(),
                              sender_addr,
                              std::string(""));

                _msgBuilder << "Player connected, UID: " << finder->player_uid();
                _logSystem.Info(_msgBuilder.str());
                _msgBuilder.str("");

                _playersPool.emplace_back(std::move(player));
            }
            _commutationNeeded = true; // player asked game, need to recompute commutation
            break;
        }

        default:
            _msgBuilder << "Undefined packet received";
            _logSystem.Warning(_msgBuilder.str());
            _msgBuilder.str("");
            break;
    }
}

void MasterServer::FreeResourcesAndSaveResults(Poco::Timer& timer)
{
    // Free resources of finished servers
    std::lock_guard<std::mutex> lock(_serversMutex);

    auto servers_freed = 0;
    _gameServers.erase(std::remove_if(_gameServers.begin(),
                                      _gameServers.end(),
                                      [this, &servers_freed](std::unique_ptr<GameServer> const& server)
                                      {
                                          if(server->GetState() == GameServer::State::FINISHED)
                                          {
                                              ++servers_freed;
                                              _availablePorts.push(server->GetConfig().Port);
                                              return true;
                                          }
                                          return false;
                                      }),
                       _gameServers.end());
}

void MasterServer::CommutatePlayers()
{
    std::lock_guard<std::mutex> lock(_serversMutex);
    // if there are some players, check that there is a server for them
    if(_playersPool.size() != 0)
    {
        auto servers_available = std::count_if(_gameServers.begin(),
                                               _gameServers.end(),
                                               [this](std::unique_ptr<GameServer> const& gs)
                                               {
                                                   return gs->GetState() == GameServer::State::LOBBY_FORMING;
                                               });

        // no servers? start a new
        if(servers_available == 0)
        {
            uint32_t nGSPort = _availablePorts.front();
            _availablePorts.pop();

            GameServer::Configuration config;
            config.Players    = 1; // +-
            config.RandomSeed = _randDistr(_randGenerator);
            config.Port       = nGSPort;

            _gameServers.emplace_back(std::make_unique<GameServer>(config));
            _threadPool->startWithPriority(Poco::Thread::Priority::PRIO_NORMAL,
                                           *_gameServers.back(),
                                           "GameServer");
        }
    }

    // FIXME: on a big amount of gameservers, may not run correctly
    for(auto& gs : _gameServers)
    {
        if(_playersPool.size() == 0) // optimization (no need to loop more through gss)
            break;

        if(gs->GetState() == GameServer::State::LOBBY_FORMING)
        {
            auto serv_config = gs->GetConfig();
            // transfer player to GS
            auto game_found  = CreateSVGameFound(_flatBuilder,
                                                 serv_config.Port);
            auto ms_event    = CreateMessage(_flatBuilder,
                                             Messages_SVGameFound,
                                             game_found.Union());
            _flatBuilder.Finish(ms_event);

            auto& player = _playersPool.front();
            
            _msgBuilder << "TRANSFER: Player " << player.GetUID() << " -> GS" << serv_config.Port;
            _logSystem.Info(_msgBuilder.str());
            _msgBuilder.str("");

            _socket.sendTo(_flatBuilder.GetBufferPointer(),
                           _flatBuilder.GetSize(),
                           player.GetAddress());

            _flatBuilder.Clear();
            _playersPool.pop_front(); // remove a player from queue
        }
    }

    _commutationNeeded = false;
}
