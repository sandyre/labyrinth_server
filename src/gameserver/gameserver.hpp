//
//  gameserver.hpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef gameserver_hpp
#define gameserver_hpp

#include "gamelogic/gameworld.hpp"
#include "ConnectionsManager.hpp"
#include "../toolkit/named_logger.hpp"
#include "../toolkit/Random.hpp"

#include <Poco/Net/DatagramSocket.h>
#include <Poco/Task.h>
#include <Poco/Timer.h>

#include <chrono>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>


class GameServer
    : public Poco::Task
{
private:
    class PlayerConnection;

public:
    static const std::chrono::microseconds PING_INTERVAL;

    enum class State
    {
        LOBBY_FORMING,
        HERO_PICK,
        GENERATING_WORLD,
        RUNNING_GAME,
        FINISHED
    };

    struct Configuration
    {
        uint32_t Port;
        uint32_t RandomSeed;
        uint16_t Players;
    };

public:
    GameServer(const Configuration&);

    virtual void runTask();

    GameServer::State GetState() const
    { return _state; }

    GameServer::Configuration GetConfig() const
    { return _config; }

private:
    void shutdown();

    void lobby_forming_stage();
    void hero_picking_stage();
    void world_generation_stage();
    void running_game_stage();

    inline bool PlayerExists(const std::string&);
    inline std::vector<PlayerConnection>::iterator FindPlayerByUID(const std::string&);

    void MessageHandler(const MessageBufferPtr& message)
    {
        std::lock_guard<std::mutex> l(_mutex);
        _messages.push_back(message);
    }

private:
    GameServer::State               _state;
    GameServer::Configuration       _config;
    std::string                     _serverName;
    std::chrono::milliseconds       _msPerUpdate;

    std::unique_ptr<GameWorld>      _world;
    std::vector<PlayerConnection>   _playersConnections;

    std::mutex                      _mutex;
    MessageStorage                  _messages;

    boost::signals2::connection     _connectionManagerConnection;
    ConnectionsManagerPtr           _connectionsManager;

    NamedLogger                     _logger;
};

#endif /* gameserver_hpp */
