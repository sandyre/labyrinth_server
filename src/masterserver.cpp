//
//  masterserver.cpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#include "masterserver.hpp"

#include "msnet_generated.h"
#include "services/DatabaseAccessor.hpp"

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

class MasterServer::RegistrationTask : public Poco::Task
{
public:
    RegistrationTask(MasterServer& masterServer,
                     const Poco::Net::SocketAddress& recipient,
                     const std::string& email,
                     const std::string& password)
    : Task("registrationTask"),
      _master(masterServer),
      _logger("RegistrationTask", NamedLogger::Mode::STDIO),
      _recipient(recipient),
      _email(email),
      _password(password)
    { }

    void runTask()
    {
        using namespace MasterEvent;

        _logger.Debug() << "Registration task acquired, waiting DatabaseAccessor response";

        DBQuery::RegisterQuery query;
        query.Email = _email;
        query.Password = _password;

        auto queryResult = DatabaseAccessor::Instance().Query(query);

        DBQuery::RegisterResult result;
        try
        {
            result = queryResult.get();
        }
        catch(const std::exception& e)
        {
            _logger.Error() << "DatabaseAccessor returned exception: " << e.what();
            setState(Poco::Task::TaskState::TASK_FINISHED);
            return;
        }

        flatbuffers::FlatBufferBuilder builder;
        if(result.Success)
        {
            _logger.Debug() << "User " << _email << " registrated successfully";
                // Send response to client
            auto response = CreateSVRegister(builder,
                                             RegistrationStatus_SUCCESS);
            auto msg      = CreateMessage(builder,
                                          Messages_SVRegister,
                                          response.Union());
            builder.Finish(msg);

            _master._socket.sendTo(builder.GetBufferPointer(),
                                   builder.GetSize(),
                                   _recipient);
        }
        else
        {
            _logger.Debug() << "User " << _email << " failed to register: email has been already taken";

                // Send response to client
            auto response = CreateSVRegister(builder,
                                             RegistrationStatus_EMAIL_TAKEN);
            auto msg      = CreateMessage(builder,
                                          Messages_SVRegister,
                                          response.Union());
            builder.Finish(msg);

            _master._socket.sendTo(builder.GetBufferPointer(),
                                   builder.GetSize(),
                                   _recipient);
        }

        setState(Poco::Task::TaskState::TASK_FINISHED);
    }

private:
    MasterServer&                                           _master;
    NamedLogger                                             _logger;
    Poco::Net::SocketAddress                                _recipient;
    const std::string                                       _email;
    const std::string                                       _password;
};

class MasterServer::LoginTask : public Poco::Task
{
public:
    LoginTask(MasterServer& masterServer,
              const Poco::Net::SocketAddress& recipient,
              const std::string& email,
              const std::string& password)
    : Task("loginTask"),
      _master(masterServer),
      _logger("LoginTask", NamedLogger::Mode::STDIO),
      _recipient(recipient),
      _email(email),
      _password(password)
    { }

    void runTask()
    {
        using namespace MasterEvent;

        _logger.Debug() << "Login task acquired, waiting DatabaseAccessor response";

        DBQuery::LoginQuery query;
        query.Email = _email;
        query.Password = _password;

        auto queryResult = DatabaseAccessor::Instance().Query(query);

        DBQuery::LoginResult result;
        try
        {
            result = queryResult.get();
        }
        catch(const std::exception& e)
        {
            _logger.Error() << "DatabaseAccessor returned exception: " << e.what();
            setState(Poco::Task::TaskState::TASK_FINISHED);
            return;
        }

        flatbuffers::FlatBufferBuilder builder;
        if(result.Success)
        {
            _logger.Debug() << "Player " << _email << " logged in";
                // Send response to client
            auto response = CreateSVLogin(builder,
                                          LoginStatus_SUCCESS);
            auto msg      = CreateMessage(builder,
                                          Messages_SVLogin,
                                          response.Union());
            builder.Finish(msg);

            _master._socket.sendTo(builder.GetBufferPointer(),
                                   builder.GetSize(),
                                   _recipient);
        }
        else // player is not registered, or wrong password
        {
            _logger.Debug() << "Player " << _email << " failed to log in: wrong pass or email";

                // Send response to client
            auto response = CreateSVLogin(builder,
                                          LoginStatus_WRONG_INPUT);
            auto msg      = CreateMessage(builder,
                                          Messages_SVLogin,
                                          response.Union());
            builder.Finish(msg);

            _master._socket.sendTo(builder.GetBufferPointer(),
                                   builder.GetSize(),
                                   _recipient);
        }

        setState(Poco::Task::TaskState::TASK_FINISHED);
    }

private:
    MasterServer&                                           _master;
    NamedLogger                                             _logger;
    Poco::Net::SocketAddress                                _recipient;
    const std::string                                       _email;
    const std::string                                       _password;
};

MasterServer::MasterServer()
: _systemStatus(0),
  _randGenerator(228),
  _randDistr(std::uniform_int_distribution<>(std::numeric_limits<int32_t>::min(),
                                             std::numeric_limits<int32_t>::max())),
  _logger("MasterServer", NamedLogger::Mode::STDIO),
  _taskWorkers("MasterServerQueryWorkers", 8, 16, 60),
  _taskManager(_taskWorkers)
{

}

MasterServer::~MasterServer()
{
    _socket.close();
    _logger.Info() << "Shutdown";
}

void MasterServer::init(uint32_t Port)
{
    // Init logging
    _systemStatus |= SystemStatus::LOG_SYSTEM_ACTIVE;

    _logger.Info() << "Booting starts";

    _logger.Info() << "Labyrinth core version: " << GAMEVERSION_MAJOR << "." << GAMEVERSION_MINOR << "." << GAMEVERSION_BUILD;

    _logger.Info() << "[------------------SYBSYSTEMS BOOTSTRAP------------------]";

    // Init Net
    _logger.Info() << "[------------------------NETWORK-------------------------]";

    // Check inet connection
    try
    {
        _logger.Debug() << "Sending HTTP request to api.ipify.org";
        
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

            _logger.Debug() << "HTTP response received";
            _logger.Debug() << "Public IP: " << rs.rdbuf();
            _logger.Debug() << "Binding to port " << Port;

            Poco::Net::SocketAddress sock_addr(Poco::Net::IPAddress(),
                                               Port);
            _socket.bind(sock_addr);

            _systemStatus |= SystemStatus::NETWORK_SYSTEM_ACTIVE;
        }
    }
    catch(const std::exception& e)
    {
        _logger.Error() << "Failed. Exception thrown: " << e.what();
    }

    // Fill ports pool
    for(uint32_t i = 1931; i < (1931 + 150); ++i)
    {
        Poco::Net::DatagramSocket socket;
        Poco::Net::SocketAddress addr(Poco::Net::IPAddress(),
                                      i);

        try
        {
            socket.bind(addr);
            _availablePorts.push(i);
            socket.close();
        }
        catch(std::exception e)
        {
            _logger.Info() << "Port " << i << " is already in use";
        }
    }

    // Init gameservers threadpool
    _logger.Warning() << "Initializing gameservers threadpool...";
    _threadPool = std::make_unique<Poco::ThreadPool>(std::thread::hardware_concurrency(), // min
                                                     std::thread::hardware_concurrency() * 4, // max
                                                     120, // idle time
                                                     0); // stack size
    _logger.Info() << "DONE";

    // Init DB
    _logger.Info() << "[---------------DATABASE ACCESSOR SERVICE----------------]";

    try
    {
        DatabaseAccessor::Instance();
        _systemStatus |= SystemStatus::DATABASE_SYSTEM_ACTIVE;
    }
    catch(const std::exception& e)
    {
        _logger.Error() << "Failed. Exception thrown: " << e.what();
    }

    if(!(SystemStatus::NETWORK_SYSTEM_ACTIVE & _systemStatus) ||
       !(SystemStatus::DATABASE_SYSTEM_ACTIVE & _systemStatus))
    {
        _logger.Error() << "[--------------SYBSYSTEMS BOOTSTRAP FAILED---------------]";
        if(!(_systemStatus & SystemStatus::NETWORK_SYSTEM_ACTIVE))
            _logger.Error() << "REASON: NETWORK SYSTEM FAILED TO START";
        if(!(_systemStatus & SystemStatus::DATABASE_SYSTEM_ACTIVE))
            _logger.Error() << "REASON: DATABASE ACCESSOR SERVICE FAILED TO START";
        exit(1);
    }

    _logger.Info() << "[---------------------SYSTEM MONITOR---------------------]";

    try
    {
        _systemMonitor = std::make_unique<SystemMonitor>();
    }
    catch(const std::exception& e)
    {
        _logger.Error() << "Failed. Exception thrown: " << e.what();
    }

    // Everything done
    _logger.Info() << "[-------------SYBSYSTEMS BOOTSTRAP COMPLETED-------------]\n\n";
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
        Poco::Net::SocketAddress sender_addr;
        
        if(_socket.available() > _dataBuffer.size())
        {
            auto pack_size = _socket.receiveFrom(_dataBuffer.data(),
                                                 _dataBuffer.size(),
                                                 sender_addr);

            _logger.Warning() << "Received packet which size is more than buffer_size. Probably, its a hack or DDoS. Sender addr: " << sender_addr.toString();
            continue;
        }
        
        auto pack_size = _socket.receiveFrom(_dataBuffer.data(),
                                             _dataBuffer.size(),
                                             sender_addr);
        
        flatbuffers::Verifier verifier(_dataBuffer.data(),
                                       pack_size);
        if(!VerifyMessageBuffer(verifier))
        {
            _logger.Warning() << "Packet verification failed, probably a DDoS. Sender addr: " << sender_addr.toString();
            continue;
        }
        
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
                if(_taskWorkers.available())
                {
                    auto registr = static_cast<const CLRegister*>(msg->message());
                    _taskManager.start(new RegistrationTask(*this,
                                                            sender_addr,
                                                            std::string(registr->email()->c_str()),
                                                            std::string(registr->password()->c_str())));
                }
                else
                    _logger.Warning() << "No workers available, task skipped";

                break;
            }
                
            case Messages_CLLogin:
            {
                if(_taskWorkers.available())
                {
                    auto login = static_cast<const CLLogin*>(msg->message());
                    _taskManager.start(new LoginTask(*this,
                                                     sender_addr,
                                                     std::string(login->email()->c_str()),
                                                     std::string(login->password()->c_str())));
                }
                else
                    _logger.Warning() << "No workers available, task skipped";

                break;
            }
                
            case Messages_CLFindGame:
            {
                auto finder = static_cast<const CLFindGame*>(msg->message());
                
                if(finder->cl_version_major() == GAMEVERSION_MAJOR)
                {
                    _logger.Info() << "Player " << finder->player_uid() << " version is OK";
                    
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
                }
                else
                {
                    _logger.Warning() << "Player " << finder->player_uid() << " connection refused: old client version"
                            << " (" << std::to_string(finder->cl_version_major()) << "."
                            << std::to_string(finder->cl_version_minor()) << "."
                            << std::to_string(finder->cl_version_build()) << ")";
                    
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
                
                GameServers::const_iterator server_available;
                {
                    server_available = std::find_if(_gameServers.cbegin(),
                                                    _gameServers.cend(),
                                                    [this](std::unique_ptr<GameServer> const& gs)
                                                    {
                                                        return gs->GetState() == GameServer::State::LOBBY_FORMING;
                                                    });
                    if(server_available == _gameServers.cend())
                    {
                        GameServer::Configuration config;
                        config.Players    = 1; // +-
                        config.RandomSeed = _randDistr(_randGenerator);
                        config.Port       = _availablePorts.front();
                        _availablePorts.pop();
                        
                        _gameServers.emplace_back(std::make_unique<GameServer>(config));
                        _threadPool->startWithPriority(Poco::Thread::Priority::PRIO_NORMAL,
                                                       *_gameServers.back(),
                                                       "GameServer");
                        server_available = _gameServers.end()-1;
                    }
                }
                
                _logger.Info() << "TRANSFER: Player " << finder->player_uid() << " -> GS" << (*server_available)->GetConfig().Port;
                
                    // transfer player to GS
                auto game_found = CreateSVGameFound(_flatBuilder,
                                                    (*server_available)->GetConfig().Port);
                auto ms_event = CreateMessage(_flatBuilder,
                                              Messages_SVGameFound,
                                              game_found.Union());
                _flatBuilder.Finish(ms_event);
                
                _socket.sendTo(_flatBuilder.GetBufferPointer(),
                               _flatBuilder.GetSize(),
                               sender_addr);
                _flatBuilder.Clear();
                break;
            }
                
            default:
                _logger.Warning() << "Undefined packet received";
                break;
        }
    }
}
