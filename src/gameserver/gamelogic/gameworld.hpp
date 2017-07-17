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
#include "units/hero.hpp"
#include "units/mage.hpp"
#include "units/monster.hpp"
#include "units/priest.hpp"
#include "units/rogue.hpp"
#include "units/warrior.hpp"
#include "../../globals.h"
#include "../../toolkit/named_logger.hpp"

#include <chrono>
#include <queue>
#include <random>
#include <sstream>
#include <vector>


class GameWorld
{
private:
    class ObjectFactory
    {
    public:
        ObjectFactory()
        : _uidSequence(1)
        { }

        template<typename T>
        std::shared_ptr<T> Create(GameWorld& world)
        {
            auto object = std::make_shared<T>(world);
            object->SetUID(_uidSequence++);

            return object;
        }

        template<typename T>
        std::shared_ptr<T> Create(GameWorld& world, uint32_t uid)
        {
            auto object = std::make_shared<T>(world);
            object->SetUID(uid);

            return object;
        }

    private:
        uint32_t    _uidSequence;
    };

public:
    enum State
    {
        RUNNING,
        PAUSE,
        FINISHED
    };

    struct PlayerInfo
    {
        uint32_t LocalUid;
        std::string Name;
        Hero::Type Hero;
    };

public:
    GameWorld(const GameMapGenerator::Configuration& conf,
              std::vector<PlayerInfo>& players);

    virtual void update(std::chrono::microseconds);

    std::queue<std::vector<uint8_t>>& GetOutgoingEvents();
    std::queue<std::vector<uint8_t>>& GetIncomingEvents();

protected:
    void        ApplyInputEvents();

    Monster *   SpawnMonster();

    Point<>     GetRandomPosition();
    
protected:
    void InitialSpawn();

    GameMapGenerator::Configuration  _mapConf;

    // contains outgoing events
    std::queue<std::vector<uint8_t>> _inputEvents;
    std::queue<std::vector<uint8_t>> _outputEvents;
    flatbuffers::FlatBufferBuilder   _flatBuilder;

    ObjectFactory               _objectFactory;
    // basicly contains all objects on scene
    std::deque<std::shared_ptr<GameObject>> _objects;

    // contains objects that should be respawned
    std::vector<std::pair<std::chrono::microseconds, Unit *>> _respawnQueue;

    // just a random generator
    std::mt19937                    _randGenerator;
    std::uniform_int_distribution<> _randDistr;

    // monster spawning timer
    std::chrono::microseconds _monsterSpawnInterval;
    std::chrono::microseconds _monsterSpawnTimer;

    // logsystem from gameserver
    NamedLogger         _logger;

    friend Unit;
    friend Monster;
    friend Hero;
    friend Mage;
    friend Priest;
    friend Warrior;
    friend Rogue;
};

#endif /* gameworld_hpp */
