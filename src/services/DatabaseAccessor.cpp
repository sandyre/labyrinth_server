//
//  DatabaseAccessor.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 09.07.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "DatabaseAccessor.hpp"

#include <Poco/Data/MySQL/Connector.h>
#include <Poco/Observer.h>

class DatabaseAccessor::RegisterTask : public Poco::Task
{
public:
    RegisterTask(Poco::Data::Session session,
                 std::unique_ptr<std::promise<DBQuery::RegisterResult>> promise,
                 const DBQuery::RegisterQuery& reg)
    : Task("registrationDatabaseTask"),
      _dbSession(session),
      _promise(std::move(promise)),
      _taskInfo(reg)
    { }

    void runTask()
    {
        using namespace Poco::Data::Keywords;

        DBQuery::RegisterResult result;
        size_t already_registered = 0;

        try
        {
            Poco::Data::Statement select(_dbSession);
            select << "SELECT COUNT(*) FROM players WHERE email=?", into(already_registered), use(_taskInfo.Email), now;
        }
        catch(...)
        {
            _promise->set_exception(std::current_exception());
            setState(Poco::Task::TaskState::TASK_FINISHED);
            return;
        }

        if(!already_registered)
        {
            try
            {
                Poco::Data::Statement insert(_dbSession);
                insert << "INSERT INTO players(email, password) VALUES(?, ?)", use(_taskInfo.Email), use(_taskInfo.Password), now;
            }
            catch(...)
            {
                _promise->set_exception(std::current_exception());
                setState(Poco::Task::TaskState::TASK_FINISHED);
                return;
            }

            result.Success = true;
        }
        else
            result.Success = false;

        _promise->set_value_at_thread_exit(result);
        setState(Poco::Task::TaskState::TASK_FINISHED);
    }

private:
    Poco::Data::Session                                      _dbSession;
    std::unique_ptr<std::promise<DBQuery::RegisterResult>>   _promise;
    DBQuery::RegisterQuery                                   _taskInfo;
};

class DatabaseAccessor::LoginTask : public Poco::Task
{
public:
    LoginTask(Poco::Data::Session session,
              std::unique_ptr<std::promise<DBQuery::LoginResult>> promise,
              const DBQuery::LoginQuery& log)
    : Task("loginDatabaseTask"),
      _dbSession(session),
      _promise(std::move(promise)),
      _taskInfo(log)
    { }

    void runTask()
    {
        using namespace Poco::Data::Keywords;

        DBQuery::LoginResult result;
        Poco::Nullable<std::string> storedPass;

        try
        {
            Poco::Data::Statement select(_dbSession);
            select << "SELECT PASSWORD FROM labyrinth.players WHERE email=?", into(storedPass), use(_taskInfo.Email), now;
        }
        catch(...)
        {
            _promise->set_exception(std::current_exception());
            setState(Poco::Task::TaskState::TASK_FINISHED);
            return;
        }

        if(!storedPass.isNull())
        {
            if(storedPass == _taskInfo.Password)
                result.Success = true;
            else
                result.Success = false;
        }
        else
            result.Success = false;

        _promise->set_value(result);
        setState(Poco::Task::TaskState::TASK_FINISHED);
    }

private:
    Poco::Data::Session                                  _dbSession;
    std::unique_ptr<std::promise<DBQuery::LoginResult>>  _promise;
    DBQuery::LoginQuery                                  _taskInfo;
};


DatabaseAccessor::DatabaseAccessor()
: _logger("DatabaseAccessor", NamedLogger::Mode::STDIO),
  _workers("DatabaseAccessorWorkers", 8, 16, 60),
  _taskManager(_workers),
  _dbSessions("MySQL", "host=127.0.0.1;user=masterserver;db=labyrinth;password=2(3oOS1E;compress=true;auto-reconnect=true", 16)
{
    Poco::Data::MySQL::Connector::registerConnector();

        // Check the database connectivity
    {
        using namespace Poco::Data::Keywords;
        size_t registered_players = 0;
        Poco::Data::Session test_session(_dbSessions.get());
        Poco::Data::Statement select(test_session);
        select << "SELECT COUNT(*) FROM players", into(registered_players), now;
    }

    _logger.Debug() << "DatabaseAccessor service is up, number of workers: " << _workers.capacity() << ", size of SessionsPool: " << _dbSessions.available();

    _taskManager.addObserver(Poco::Observer<ProgressHandler, Poco::TaskStartedNotification>(_progressHandler,
                                                                                            &ProgressHandler::onStarted));
    _taskManager.addObserver(Poco::Observer<ProgressHandler, Poco::TaskFinishedNotification>(_progressHandler,
                                                                                             &ProgressHandler::onFinished));
}

std::future<DBQuery::RegisterResult>
DatabaseAccessor::Query(const DBQuery::RegisterQuery& reg)
{
    std::lock_guard<std::mutex> l(_callGuard);

    auto promise = std::make_unique<std::promise<DBQuery::RegisterResult>>();
    auto future = promise->get_future();
    try
    {
        _taskManager.start(new RegisterTask(_dbSessions.get(), std::move(promise), reg));
    }
    catch(const std::exception& e)
    {
        promise->set_exception(std::current_exception());
    }

    return future;
}

std::future<DBQuery::LoginResult>
DatabaseAccessor::Query(const DBQuery::LoginQuery& login)
{
    std::lock_guard<std::mutex> l(_callGuard);

    auto promise = std::make_unique<std::promise<DBQuery::LoginResult>>();
    auto future = promise->get_future();
    try
    {
        _taskManager.start(new LoginTask(_dbSessions.get(), std::move(promise), login));
    }
    catch(const std::exception& e)
    {
        promise->set_exception(std::current_exception());
    }

    return future;
}
