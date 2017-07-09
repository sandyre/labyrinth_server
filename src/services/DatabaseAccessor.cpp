//
//  DatabaseAccessor.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 09.07.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "DatabaseAccessor.hpp"

#include <Poco/Data/MySQL/Connector.h>

DatabaseAccessor::DatabaseAccessor()
: _logger("DatabaseAccessor", NamedLogger::Mode::STDIO),
  _workers("DatabaseAccessorWorkers"),
  _taskManager(_workers)
{
    Poco::Data::MySQL::Connector::registerConnector();
    std::string con_params = "host=127.0.0.1;user=masterserver;db=labyrinth;password=2(3oOS1E;compress=true;auto-reconnect=true";
    _db = std::make_unique<Poco::Data::Session>("MySQL",
                                                con_params);

    _logger.Info() << "DatabaseAccessor service is up, number of workers: " << _workers.capacity() << End();
}

std::future<DBQuery::RegisterResult>
DatabaseAccessor::Query(const DBQuery::RegisterQuery& reg)
{
    std::shared_ptr<std::promise<DBQuery::RegisterResult>> promise = std::make_shared<std::promise<DBQuery::RegisterResult>>();
    RegisterTask * regTask = new RegisterTask(_db, promise, reg);
    _taskManager.start(regTask);

    return promise->get_future();
}

std::future<DBQuery::LoginResult>
DatabaseAccessor::Query(const DBQuery::LoginQuery& login)
{
    auto promise = std::make_shared<std::promise<DBQuery::LoginResult>>();
    LoginTask * loginTask = new LoginTask(_db, promise, login);
    _taskManager.start(loginTask);

    return promise->get_future();
}
