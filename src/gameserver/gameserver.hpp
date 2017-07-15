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
#include "../toolkit/named_logger.hpp"
#include "../toolkit/Random.hpp"

#include <Poco/Net/DatagramSocket.h>
#include <Poco/Task.h>
#include <Poco/Timer.h>

#include <chrono>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

class GameServer : public Poco::Task
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

    void Ping();

    void SendSingle(flatbuffers::FlatBufferBuilder& builder,
                    Poco::Net::SocketAddress& address);
    void SendMulticast(const std::vector<uint8_t>& buffer);
    void SendMulticast(flatbuffers::FlatBufferBuilder& builder);

    inline bool PlayerExists(const std::string&);
    inline std::vector<PlayerConnection>::iterator FindPlayerByUID(const std::string&);

private:
    GameServer::State               _state;
    GameServer::Configuration       _config;
    std::string                     _serverName;
    Poco::Net::DatagramSocket       _socket;
    std::chrono::milliseconds       _msPerUpdate;

    std::unique_ptr<GameWorld>      _world;
    std::vector<PlayerConnection>   _playersConnections;

    NamedLogger                     _logger;
};

#endif /* gameserver_hpp */
