//
//  gameworld.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef gameworld_hpp
#define gameworld_hpp

#include "construction.hpp"
#include "gamemap.hpp"
#include "gameobject.hpp"
#include "units/hero.hpp"
#include "units/mage.hpp"
#include "units/monster.hpp"
#include "units/warrior.hpp"
#include "../../globals.h"
#include "../../toolkit/named_logger.hpp"
#include "../../toolkit/Random.hpp"

#include <chrono>
#include <list>
#include <set>
#include <queue>
#include <random>
#include <sstream>
#include <vector>
using namespace std::chrono_literals;

class GameWorld
{
private:
    class ObjectsStorage
    {
        using Storage = std::list<GameObjectPtr>;
    public:
        ObjectsStorage(GameWorld& world)
        : _world(world),
          _uidSeq()
        { }

        template<typename T>
        std::shared_ptr<T> Create()
        {
            auto object = std::make_shared<T>(_world);
            object->SetUID(_uidSeq++);
            _storage.push_back(object);

            return object;
        }

        template<typename T>
        std::shared_ptr<T> Create(uint32_t uid)
        {
#ifdef _DEBUG
            // Consistency check (uid should not have duplicates)
            bool is_copy = std::any_of(_storage.begin(),
                                       _storage.end(),
                                       [uid](const GameObjectPtr& obj)
                                       {
                                           return uid == obj->GetUID();
                                       });
            assert(!is_copy);
#endif
            auto object = std::make_shared<T>(_world);
            object->SetUID(uid);
            _storage.push_back(object);

            return object;
        }

        /*
         * description: Prefer using Create<> to Create-and-add object to the storage
         * use Push ONLY if it was created by Create<>, but suddenly was removed from the storage (Item mechanics)
         */
        void PushObject(const GameObjectPtr& obj)
        { _storage.push_back(obj); }

        void DeleteObject(const GameObjectPtr& obj)
        { _storage.remove(obj); }

        template<typename T>
        std::vector<std::shared_ptr<T>> Subset()
        {
            std::vector<std::shared_ptr<T>> result;
            for(auto& obj : _storage)
                if(auto cast = std::dynamic_pointer_cast<T>(obj))
                    result.push_back(cast);
            return result;
        }

        template<typename T = GameObject>
        std::shared_ptr<T> FindObject(uint32_t uid)
        {
            auto iter = std::find_if(_storage.begin(),
                                     _storage.end(),
                                     [uid](const GameObjectPtr& obj)
                                     {
                                         return obj->GetUID() == uid;
                                     });
            if(iter != _storage.end())
                return std::dynamic_pointer_cast<T>(*iter);

            return nullptr;
        }

        /*
         * STL-containers like API
         */

        Storage::iterator Begin()
        { return _storage.begin(); }
        
        Storage::iterator End()
        { return _storage.end(); }

        size_t Size() const
        { return _storage.size(); }
        
    private:
        GameWorld&                  _world;
        uint32_t                    _uidSeq;
        std::list<GameObjectPtr>    _storage;
    };

    class Respawner
    {
        using QueueElement = std::pair<std::chrono::microseconds, UnitPtr>;
    public:
        Respawner(GameWorld& world)
        : _world(world)
        { }

        void update(std::chrono::microseconds delta)
        {
            std::for_each(_queue.begin(),
                          _queue.end(),
                          [delta](QueueElement& elem)
                          {
                              elem.first -= delta;
                          });
            _queue.erase(std::remove_if(_queue.begin(),
                                        _queue.end(),
                                        [this](QueueElement& elem)
                                        {
                                            if(elem.first <= 0s)
                                            {
                                                auto grave = _world._objectsStorage.Subset<Graveyard>();

                                                _world._objectsStorage.PushObject(elem.second);
                                                elem.second->Spawn(grave[0]->GetPosition());
                                                return true;
                                            }

                                            return false;
                                        }),
                         _queue.end());

        }

        void Enqueue(const UnitPtr& unit, std::chrono::microseconds respawnTime = 5s)
        { _queue.push_back(std::make_pair(respawnTime, unit)); }

    private:
        GameWorld&                  _world;
        std::deque<QueueElement>    _queue;
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

    std::queue<std::vector<uint8_t>>& GetOutgoingEvents()
    { return _outputEvents; }
    
    void PushEvent(const std::vector<uint8_t>& event)
    { _inputEvents.push(event); }

protected:
    void ApplyInputEvents();

    Point<> GetRandomPosition();
    
    void InitialSpawn();

protected:
    GameMapGenerator::Configuration  _mapConf;

    // contains outgoing events
    std::queue<std::vector<uint8_t>> _inputEvents;
    std::queue<std::vector<uint8_t>> _outputEvents;

    ObjectsStorage              _objectsStorage;
    Respawner                   _respawner;

    // monster spawning timer
    std::chrono::microseconds _monsterSpawnInterval;
    std::chrono::microseconds _monsterSpawnTimer;

    RandomGenerator<std::mt19937, std::uniform_int_distribution<>> _randGen;
    NamedLogger         _logger;

    friend Unit;
    friend Monster;
    friend Hero;
    friend Mage;
    friend Warrior;

    friend Respawner;
};

#endif /* gameworld_hpp */
