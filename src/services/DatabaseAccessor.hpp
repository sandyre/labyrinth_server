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

#include <Poco/Data/SessionFactory.h>
#include <Poco/TaskManager.h>
#include <Poco/ThreadPool.h>

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
    class RegisterTask : public Poco::Task
    {
    public:
        RegisterTask(std::shared_ptr<Poco::Data::Session> db,
                     std::shared_ptr<std::promise<DBQuery::RegisterResult>> promise,
                     const DBQuery::RegisterQuery& reg)
        : Task("registrationTask"),
          _db(db),
          _logger("RegistrationTask", NamedLogger::Mode::STDIO),
          _promise(promise),
          _taskInfo(reg)
        { }

        void runTask()
        {
            using namespace Poco::Data::Keywords;

            DBQuery::RegisterResult result;
            size_t already_registered = 0;

            _logger.Info() << "Processing SQL register query (select)" << End();

            Poco::Data::Statement select(*_db);
            select << "SELECT COUNT(*) FROM players WHERE email=?", into(already_registered), use(_taskInfo.Email), now;

            if(!already_registered)
            {
                _logger.Info() << "Processing SQL register query (insert)" << End();

                Poco::Data::Statement insert(*_db);
                insert << "INSERT INTO players(email, password) VALUES(?, ?)", use(_taskInfo.Email), use(_taskInfo.Password), now;

                result.Success = true;
            }
            else
                result.Success = false;
            
            _promise->set_value(result);
            setProgress(1.0f);
        }

    private:
        NamedLogger                                              _logger;
        std::shared_ptr<Poco::Data::Session>                     _db;
        std::shared_ptr<std::promise<DBQuery::RegisterResult>>   _promise;
        DBQuery::RegisterQuery                                   _taskInfo;
    };

    class LoginTask : public Poco::Task
    {
    public:
        LoginTask(std::shared_ptr<Poco::Data::Session> db,
                  std::shared_ptr<std::promise<DBQuery::LoginResult>> promise,
                  const DBQuery::LoginQuery& log)
        : Task("loginTask"),
          _db(db),
          _logger("RegistrationTask", NamedLogger::Mode::STDIO),
          _promise(promise),
          _taskInfo(log)
        { }

        void runTask()
        {
            using namespace Poco::Data::Keywords;

            DBQuery::LoginResult result;
            Poco::Nullable<std::string> storedPass;

            _logger.Info() << "Processing SQL query (select)" << End();

            Poco::Data::Statement select(*_db);
            select << "SELECT PASSWORD FROM labyrinth.players WHERE email=?", into(storedPass), use(_taskInfo.Email), now;

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
            setProgress(1.0f);
        }

    private:
        NamedLogger                                          _logger;
        std::shared_ptr<Poco::Data::Session>                 _db;
        std::shared_ptr<std::promise<DBQuery::LoginResult>>  _promise;
        DBQuery::LoginQuery                                  _taskInfo;
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

    Poco::TaskManager                       _taskManager;
    Poco::ThreadPool                        _workers;

    std::shared_ptr<Poco::Data::Session>    _db;
};


#endif /* DatabaseAccessor_hpp */
