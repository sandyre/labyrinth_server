//
//  DatabaseAccessor.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 09.07.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef DatabaseAccessor_hpp
#define DatabaseAccessor_hpp

#include "../toolkit/named_logger.hpp"

#include <Poco/Data/SessionPool.h>
#include <Poco/TaskManager.h>
#include <Poco/ThreadPool.h>
#include <Poco/TaskNotification.h>

#include <future>

namespace DBQuery
{
    struct RegisterQuery
    {
        std::string Email;
        std::string Password;
    };
    struct RegisterResult
    {
        bool Success;
    };

    struct LoginQuery
    {
        std::string Email;
        std::string Password;
    };
    struct LoginResult
    {
        bool Success;
    };
}

class DatabaseAccessor
{
private:
    class RegisterTask;
    class LoginTask;

    class ProgressHandler
    {
    public:
        ProgressHandler()
        : _logger("TasksProgressReporter", NamedLogger::Mode::STDIO)
        { }

        void onStarted(Poco::TaskStartedNotification* pNf)
        {
            _logger.Debug() << pNf->task()->name() << " started.";
            pNf->release();
        }

        void onFinished(Poco::TaskFinishedNotification* pNf)
        {
            _logger.Debug() << pNf->task()->name() << " finished.";
            pNf->release();
        }

    private:
        NamedLogger             _logger;
    };

public:
    static DatabaseAccessor& Instance()
    {
        static DatabaseAccessor dbAccessor;
        return dbAccessor;
    }

    std::future<DBQuery::RegisterResult> Query(const DBQuery::RegisterQuery&);
    std::future<DBQuery::LoginResult> Query(const DBQuery::LoginQuery&);

private:
    DatabaseAccessor();

private:
    NamedLogger                             _logger;

    std::mutex                              _callGuard;
    Poco::TaskManager                       _taskManager;
    Poco::ThreadPool                        _workers;
    ProgressHandler                         _progressHandler;

    Poco::Data::SessionPool                 _dbSessions;
};


#endif /* DatabaseAccessor_hpp */
