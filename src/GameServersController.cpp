//
//  GameServersController.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 10.07.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "GameServersController.hpp"

#include <Poco/Observer.h>


GameServersController::GameServersController()
: _logger("GameServersController", NamedLogger::Mode::STDIO),
  _workers("GameServerWorker"),
  _taskManager(_workers)
{
    _logger.Debug() << "GameServerController is up, number of workers: " << _workers.available();

    for(auto idx = 1931; idx < 1931 + _workers.available(); ++idx)
        _availablePorts.push_back(idx);

    using namespace Poco;
    _taskManager.addObserver(Observer<GameServersController, TaskStartedNotification>(*this,
                                                                                      &GameServersController::onStarted));
    _taskManager.addObserver(Observer<GameServersController, TaskFinishedNotification>(*this,
                                                                                       &GameServersController::onFinished));
}

GameServersController::~GameServersController()
{
    _logger.Info() << "Waiting for all workers to end";
    _workers.collect();
    _logger.Info() << "Shutdown";
}

    // TODO: should return future<uint16_t>, and callee wait in queue for available server.
std::experimental::optional<uint16_t>
GameServersController::GetServerAddress()
{
    std::experimental::optional<uint16_t> result;
    auto list = _taskManager.taskList();
    for(auto task : list)
    {
	GameServer * server = dynamic_cast<GameServer*>(task.get());
	if (server->GetState() == GameServer::State::LOBBY_FORMING)
	    return server->GetConfig().Port;
    }

        // If no servers available - start new one and return its address
    if(!result && _workers.available())
    {
        GameServer::Configuration config;
        config.Players = 1;
        config.RandomSeed = 0;

        {
            std::lock_guard<std::mutex> lock(_availablePortsMutex);
            config.Port = _availablePorts.back();
            _availablePorts.pop_back();
        }
        _taskManager.start(new GameServer(config));

        result = config.Port;
    }

    return result;
}
