//
//  gameworld.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef gameworld_hpp
#define gameworld_hpp

#include "gamemap.hpp"
#include "gameobject.hpp"
#include "../globals.h"
#include "../logsystem.hpp"
#include "../player.hpp"
#include "units/hero.hpp"
#include "units/mage.hpp"
#include "units/monster.hpp"
#include "units/priest.hpp"
#include "units/rogue.hpp"
#include "units/warrior.hpp"

#include <chrono>
#include <queue>
#include <random>
#include <sstream>
#include <vector>

class GameWorld
{
public:
    enum State
    {
        RUNNING,
        PAUSE,
        FINISHED
    };
public:
    GameWorld();
    ~GameWorld();

    virtual void update(std::chrono::microseconds);

    void AddPlayer(Player);
    void CreateGameMap(const GameMap::Configuration&);
    void InitialSpawn();

    std::queue<std::vector<uint8_t>>& GetOutgoingEvents();
    std::queue<std::vector<uint8_t>>& GetIncomingEvents();
protected:
    void        ApplyInputEvents();

    Monster *   SpawnMonster();

    Point2      GetRandomPosition();
protected:
    GameMap::Configuration m_stMapConf;

    // contains outgoing events
    std::queue<std::vector<uint8_t>> _inputEvents;
    std::queue<std::vector<uint8_t>> _outputEvents;
    flatbuffers::FlatBufferBuilder   _flatBuilder;

    // basicly contains all objects on scene
    std::vector<GameObject*>    _objects;
    uint32_t                    _objUIDSeq;

    // contains objects that should be respawned
    std::vector<std::pair<std::chrono::microseconds, Unit *>> _respawnQueue;

    // just a random generator
    std::mt19937                    _randGenerator;
    std::uniform_int_distribution<> _randDistr;

    // monster spawning timer
    std::chrono::microseconds _monsterSpawnInterval;
    std::chrono::microseconds _monsterSpawnTimer;

    // logsystem from gameserver
    LogSystem           _logSystem;
    std::ostringstream  _logBuilder;

    friend GameMap;
    friend Unit;
    friend Monster;
    friend Hero;
    friend Mage;
    friend Priest;
    friend Warrior;
    friend Rogue;
};

#endif /* gameworld_hpp */
