//
//  GameServersController.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 10.07.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef GameServersController_hpp
#define GameServersController_hpp

#include "gameserver/gameserver.hpp"
#include "toolkit/named_logger.hpp"
#include "toolkit/optional.hpp"

#include <Poco/TaskManager.h>
#include <Poco/TaskNotification.h>
#include <Poco/ThreadPool.h>

#include <deque>


class GameServersController
{
public:
    GameServersController();
    ~GameServersController();

    std::experimental::optional<uint16_t> GetServerAddress();

private:
    void onStarted(Poco::TaskStartedNotification* pNf)
    {
        _logger.Debug() << pNf->task()->name() << " started.";
        pNf->release();
    }

    void onFinished(Poco::TaskFinishedNotification* pNf)
    {
        std::lock_guard<std::mutex> l(_availablePortsMutex);
        _availablePorts.push_back(dynamic_cast<GameServer*>(pNf->task())->GetConfig().Port);

        _logger.Debug() << pNf->task()->name() << " finished.";
        pNf->release();
    }
    
private:
    NamedLogger             _logger;

    std::mutex              _availablePortsMutex;
    std::deque<uint16_t>    _availablePorts;

    Poco::TaskManager       _taskManager;
    Poco::ThreadPool        _workers;
};

#endif /* GameServersController_hpp */
