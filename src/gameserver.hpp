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
#include <Poco/Runnable.h>
#include <Poco/Timer.h>

#include <chrono>
#include <cstring>
#include <memory>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

using std::chrono::steady_clock;

class GameServer : public Poco::Runnable
{
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
    ~GameServer();

    virtual void run();

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
    void SendMulticast(const std::vector<uint8_t>&);

    inline std::vector<Player>::iterator FindPlayerByUID(PlayerUID);

private:
    GameServer::State         _state;
    GameServer::Configuration _config;
    std::string               _serverName;
    Poco::Net::DatagramSocket _socket;
    steady_clock::time_point  _startTime;
    std::chrono::milliseconds _msPerUpdate;

    std::unique_ptr<Poco::Timer>    _pingerTimer;
    
    std::unique_ptr<GameWorld> _gameWorld;
    std::mutex                 _playersMutex;
    std::vector<Player>        _players;

    NamedLogger                _logger;

    flatbuffers::FlatBufferBuilder _flatBuilder;
};

#endif /* gameserver_hpp */
