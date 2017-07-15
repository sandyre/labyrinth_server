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
#include "player.hpp"
#include "toolkit/named_logger.hpp"

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

using std::chrono::steady_clock;

class GameServer : public Poco::Task
{
private:
    class PlayerConnection;

public:
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

    void update();
    void Ping(Poco::Timer&);

    void SendSingle(flatbuffers::FlatBufferBuilder& builder,
                    Poco::Net::SocketAddress& address);
    void SendMulticast(const std::vector<uint8_t>& buffer);
    void SendMulticast(flatbuffers::FlatBufferBuilder& builder);

    inline bool PlayerExists(PlayerUID);
    inline std::vector<PlayerConnection>::iterator FindPlayerByUID(PlayerUID);

private:
    GameServer::State               _state;
    GameServer::Configuration       _config;
    std::string                     _serverName;
    Poco::Net::DatagramSocket       _socket;
    steady_clock::time_point        _startTime;
    std::chrono::milliseconds       _msPerUpdate;

    std::unique_ptr<Poco::Timer>    _pingerTimer;

    std::unique_ptr<GameWorld>      _world;
    std::recursive_mutex            _playersMutex;
    std::vector<PlayerConnection>   _playersConnections;

    NamedLogger                     _logger;
};

#endif /* gameserver_hpp */
