//
//  GameServersController.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 10.07.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef GameServersController_hpp
#define GameServersController_hpp

#include "gameserver.hpp"
#include "toolkit/named_logger.hpp"

#include <Poco/TaskManager.h>
#include <Poco/TaskNotification.h>
#include <Poco/ThreadPool.h>

#include "toolkit/optional.hpp"
#include <deque>

class GameServersController
{
public:
    GameServersController();
    ~GameServersController()
    {
        _logger.Info() << "Waiting for all workers to end";
        _workers.collect();
        _logger.Info() << "Shutdown";
    }

    std::experimental::optional<uint16_t> GetServerAddress();

private:
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

    Poco::TaskManager       _taskManager;
    Poco::ThreadPool        _workers;
};

#endif /* GameServersController_hpp */
