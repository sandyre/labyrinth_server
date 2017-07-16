//
//  masterserver.cpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#include "masterserver.hpp"

#include "MasterMessage.h"
#include "toolkit/SafePacketGetter.hpp"
#include "services/DatabaseAccessor.hpp"

#include <Poco/Environment.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/URI.h>

#include <chrono>
#include <limits>
#include <memory>
#include <utility>

using namespace MasterMessage;
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
    : Task("RegistrationTask"),
      _master(masterServer),
      _logger("RegistrationTask", NamedLogger::Mode::STDIO),
      _recipient(recipient),
      _email(email),
      _password(password)
    { }

    void runTask()
    {
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
            auto msg = CreateMessage(builder,
                                     0,
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
            auto msg = CreateMessage(builder,
                                     0,
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
    : Task("LoginTask"),
      _master(masterServer),
      _logger("LoginTask", NamedLogger::Mode::STDIO),
      _recipient(recipient),
      _email(email),
      _password(password)
    { }

    void runTask()
    {
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
            auto msg = CreateMessage(builder,
                                     0,
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
            auto msg = CreateMessage(builder,
                                     0,
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

class MasterServer::FindGameTask : public Poco::Task
{
public:
    FindGameTask(MasterServer& masterServer,
                 const Poco::Net::SocketAddress& recipient)
    : Task("FindGameTask"),
    _master(masterServer),
    _logger("FindGameTask", NamedLogger::Mode::STDIO),
    _recipient(recipient)
    { }

    void runTask()
    {
        _logger.Debug() << "FindGame task acquired, waiting GameServersController response";

        auto serverPort = _master._gameserversController->GetServerAddress();
        if(!serverPort)
        {
            _logger.Warning() << "GameServersController returned no address (no servers available)";
            setState(Poco::Task::TaskState::TASK_FINISHED);
            return;
        }

        flatbuffers::FlatBufferBuilder builder;
        _logger.Debug() << "Found game for [" << _recipient.toString() << "]";

        auto game_found = CreateSVGameFound(builder,
                                            *serverPort);
        auto ms_event = CreateMessage(builder,
                                      0,
                                      Messages_SVGameFound,
                                      game_found.Union());
        builder.Finish(ms_event);

        _master._socket.sendTo(builder.GetBufferPointer(),
                               builder.GetSize(),
                               _recipient);

        setState(Poco::Task::TaskState::TASK_FINISHED);
    }

private:
    MasterServer&                                           _master;
    NamedLogger                                             _logger;
    Poco::Net::SocketAddress                                _recipient;
};


MasterServer::MasterServer()
: _logger("MasterServer", NamedLogger::Mode::STDIO),
  _taskWorkers("MasterServerQueryWorkers", 8, 16, 60),
  _taskManager(_taskWorkers)
{
    uint16_t Port = 1930;
    _logger.Info() << "Booting starts";

    _logger.Info() << "Labyrinth core version: " << GAMECORE_MAJOR_VERSION << "." << GAMECORE_MINOR_VERSION << "." << GAMECORE_BUILD_VERSION;
    _logger.Info() << "[----------------------PLATFORM INFO---------------------]";
    {
        _logger.Info() << "Operating System: " << Poco::Environment::osDisplayName() << " "
        << Poco::Environment::osVersion() << " "
        << Poco::Environment::osArchitecture();
        _logger.Info() << "MasterServer linked against POCO version: "
        << ((Poco::Environment::libraryVersion() >> 24) & 0xFF) << "."
        << ((Poco::Environment::libraryVersion() >> 16) & 0xFF) << "."
        << ((Poco::Environment::libraryVersion() >> 8) & 0xFF) << "."
        << ((Poco::Environment::libraryVersion() & 0xFF));
    }

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
        }
    }
    catch(const std::exception& e)
    {
        _logger.Error() << "Failed. Exception thrown: " << e.what();
        _logger.Error() << "[--------------SYBSYSTEMS BOOTSTRAP FAILED---------------]";

        exit(1);
    }

        // Init gameservers threadpool
    _logger.Info() << "[----------------GAME SERVERS CONTROLLER-----------------]";
    try
    {
        _gameserversController = std::make_unique<GameServersController>();
    }
    catch(const std::exception& e)
    {
        _logger.Error() << "Failed. Exception thrown: " << e.what();
    }

        // Init DB
    _logger.Info() << "[---------------DATABASE ACCESSOR SERVICE----------------]";

    try
    {
        DatabaseAccessor::Instance();
    }
    catch(const std::exception& e)
    {
        _logger.Error() << "Failed. Exception thrown: " << e.what();
        _logger.Error() << "[--------------SYBSYSTEMS BOOTSTRAP FAILED---------------]";

        exit(2);
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

MasterServer::~MasterServer()
{
    _logger.Info() << "Waiting all workers to complete";
    _taskWorkers.collect();
}


void MasterServer::run()
{
    _logger.Info() << "[----------------MASTERSERVER IS RUNNING-----------------]";
    while(true)
    {
        SafePacketGetter packetGetter(_socket);
        auto packet = packetGetter.Get<MasterMessage::Message>();
        if(!packet)
            continue;

        auto msg = GetMessage(packet->Data.data());

        switch(msg->payload_type())
        {
        case Messages_CLPing:
        {
            flatbuffers::FlatBufferBuilder builder;
            auto pong = CreateSVPing(builder);
            auto smsg = CreateMessage(builder,
                                      0,
                                      Messages_CLPing,
                                      pong.Union());
            builder.Finish(smsg);

            _socket.sendTo(builder.GetBufferPointer(),
                           builder.GetSize(),
                           packet->Sender);
            break;
        }

        case Messages_CLRegister:
        {
            if(_taskWorkers.available())
            {
                auto registr = static_cast<const CLRegister*>(msg->payload());
                _taskManager.start(new RegistrationTask(*this,
                                                        packet->Sender,
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
                auto login = static_cast<const CLLogin*>(msg->payload());
                _taskManager.start(new LoginTask(*this,
                                                 packet->Sender,
                                                 std::string(login->email()->c_str()),
                                                 std::string(login->password()->c_str())));
            }
            else
                _logger.Warning() << "No workers available, task skipped";

            break;
        }

        case Messages_CLFindGame:
        {
            if(_taskWorkers.available())
            {
                auto finder = static_cast<const CLFindGame*>(msg->payload());
                _taskManager.start(new FindGameTask(*this,
                                                    packet->Sender));
            }
            else
                _logger.Warning() << "No workers available, task skipped";

            break;
        }

        default:
            _logger.Warning() << "Undefined packet received";
            break;
        }
    }
}
