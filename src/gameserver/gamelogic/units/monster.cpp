//
//  monster.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "monster.hpp"

#include "../gameworld.hpp"
#include "../../GameMessage.h"
#include "../../../toolkit/AStar.hpp"

#include <chrono>
using namespace std::chrono_literals;


Monster::Monster(GameWorld& world, uint32_t uid)
: Unit(world, uid)
{
    _unitType = Unit::Type::MONSTER;
    _name = "Skeleton";

    _health = SimpleProperty<>(50, 0, 50);
    _damage = SimpleProperty<>(10, 0, 100);
    _armor = SimpleProperty<>(2, 0, 100);
    _resistance = SimpleProperty<>(2, 0, 100);

        // spell 0 - movement
    _cdManager.AddSpell(1s);

        // spell 1 cd
    _cdManager.AddSpell(3s);
    
        // spell 1 seq
    InputSequence atk_seq(5);
    _castSequence.push_back(atk_seq);
    
    _castTime = 600ms;
    _castATime = 0ms;
}


void
Monster::update(std::chrono::microseconds delta)
{
    Unit::update(delta);
    
    _castATime -= delta;
    if (_castATime < 0ms)
        _castATime = 0ms;

    if (!(_unitAttributes & Unit::Attributes::INPUT))
        return;
    
    switch (_state)
    {
    case Unit::State::WALKING:
    {
        if (!_chasingUnit)
        {
            // check nearby area for enemies
            bool targetFound = false;

            auto units = _world._objectsStorage.Subset<Unit>();
            for (auto& unit : units)
            {
                if(unit->GetUID() != this->GetUID() &&
                   unit->GetType() != Unit::Type::MONSTER &&
                   unit->GetState() == Unit::State::WALKING &&
                   unit->GetPosition().Distance(this->GetPosition()) <= 6.0)
                {
                    _logger.Info() << "Begin chasing " << unit->GetName();
                    _chasingUnit = unit;
                    targetFound = true;
                    break;
                }
            }

            // no enemies - sleep
            if (!targetFound)
                break;
        }

        // check path
        if ((*_chasingUnit)->GetPosition().Distance(this->GetPosition()) > 6.0 ||
            (*_chasingUnit)->GetState() != Unit::State::WALKING)
        {
            _logger.Info() << "End chasing " << (*_chasingUnit)->GetName();
            _chasingUnit.reset();
            _pathToUnit.reset();
            break;
        }
        else if (!_pathToUnit
                 || (*_chasingUnit)->GetPosition() != _pathToUnit->back()) // check re/-calculation need
        {
            // it means that object moved since last update. recalculation needed
            auto mapSize = _world._mapConf.MapSize * _world._mapConf.RoomSize + 2;
            std::vector<std::vector<int8_t>> binary_world(mapSize, std::vector<int8_t>(mapSize, 1));

            // iterate through all objects in the world and mark unpassable cells
            auto objects = _world._objectsStorage.Subset<GameObject>();
            for(auto obj : objects)
            {
                if(!(obj->GetAttributes() & GameObject::Attributes::PASSABLE))
                {
                    auto objPos = obj->GetPosition();
                    binary_world[objPos.x][objPos.y] = 0;
                }
            }

            // make itself AND target cells passable
            binary_world[this->GetPosition().x][this->GetPosition().y] = 1;
            binary_world[(*_chasingUnit)->GetPosition().x][(*_chasingUnit)->GetPosition().y] = 1;

            auto path = AStar(binary_world, this->GetPosition(), (*_chasingUnit)->GetPosition());
            if (path) // path found!
            {
                std::ostringstream path_str;
                for(auto& pt : *path)
                    path_str << pt;

                _logger.Debug() << "Path found: " << path_str.str();

                _pathToUnit = path;
            }
            else
                _logger.Debug() << "Failed to find path";
        }

        if ((*_chasingUnit)->GetPosition().Distance(this->GetPosition()) == 1.0
            && _cdManager.SpellReady(0))
        {
            this->StartDuel(*_chasingUnit);
            _chasingUnit.reset();
            _pathToUnit.reset();
            break;
        }
        else if (_pathToUnit && _cdManager.SpellReady(0))
        {
            const Point<> nextPos = _pathToUnit->front();

            if (nextPos.x > _pos.x)
                Move(Unit::MoveDirection::RIGHT);
            else if (nextPos.x < _pos.x)
                Move(Unit::MoveDirection::LEFT);
            else if (nextPos.y > _pos.y)
                Move(Unit::MoveDirection::UP);
            else if (nextPos.y < _pos.y)
                Move(Unit::MoveDirection::DOWN);

            _cdManager.Restart(0);
            _pathToUnit->pop_front();
        }

        break;
    }

    case Unit::State::DUEL:
    {
        if (_castATime == 0ms)
        {
            _castSequence[0].sequence.pop_back();
            _castATime = _castTime;
            
            if (const auto enemy = _duelTarget.lock();
                enemy && _castSequence[0].sequence.empty())
            {
                    // Log damage event
                _logger.Info() << "Attack " << enemy->GetName() << " for " << _damage << " physical damage";
                
                    // set up CD
                _cdManager.Restart(0);
                
                flatbuffers::FlatBufferBuilder builder;
                auto spell_info = GameMessage::CreateMonsterAttack(builder,
                                                                   enemy->GetUID(),
                                                                   _damage);
                auto spell = GameMessage::CreateSpell(builder,
                                                      GameMessage::Spells_MonsterAttack,
                                                      spell_info.Union());
                auto spell1 = GameMessage::CreateSVActionSpell(builder,
                                                               this->GetUID(),
                                                               0,
                                                               spell);
                auto event = GameMessage::CreateMessage(builder,
                                                        0,
                                                        GameMessage::Messages_SVActionSpell,
                                                        spell1.Union());
                builder.Finish(event);
                
                _world._outputMessages.push_back(std::make_shared<MessageBuffer>(builder.GetCurrentBufferPointer(),
                                                                                 builder.GetBufferPointer() + builder.GetSize()));
                
                    // deal PHYSICAL damage
                auto dmgDescr = Unit::DamageDescriptor();
                dmgDescr.DealerName = _name;
                dmgDescr.Value = _damage;
                dmgDescr.Type = Unit::DamageDescriptor::DamageType::PHYSICAL;
                enemy->TakeDamage(dmgDescr);
                
                _castSequence[0].Refresh();
            }
        }
        
        break;
    }
        
    default:
        break;
    }
}


void
Monster::Spawn(const Point<>& pos)
{
    _logger.Info() << "Spawned at " << pos;

    _state = Unit::State::WALKING;
    _objAttributes = GameObject::Attributes::MOVABLE | GameObject::Attributes::VISIBLE | GameObject::Attributes::DAMAGABLE;
    _unitAttributes = Unit::Attributes::INPUT | Unit::Attributes::ATTACK | Unit::Attributes::DUELABLE;
    _health = _health.Max();
    _pos = pos;
    
    flatbuffers::FlatBufferBuilder builder;
    auto spawn = GameMessage::CreateSVSpawnMonster(builder,
                                                   this->GetUID(),
                                                   pos.x,
                                                   pos.y);
    auto msg = GameMessage::CreateMessage(builder,
                                          0,
                                          GameMessage::Messages_SVSpawnMonster,
                                          spawn.Union());
    builder.Finish(msg);
    
    _world._outputMessages.push_back(std::make_shared<MessageBuffer>(builder.GetCurrentBufferPointer(),
                                                                     builder.GetBufferPointer() + builder.GetSize()));
}


void
Monster::Die(const std::string& killerName)
{
        // Log death event
    _logger.Info() << "Killed by " << killerName << " at " << _pos;
    
    // drop items
    auto items = _inventory;
    for(auto item : items)
        this->DropItem(item->GetUID());


    if(const auto enemy = _duelTarget.lock())
    {
        enemy->EndDuel();
        EndDuel();
    }
    
    flatbuffers::FlatBufferBuilder builder;
    auto move = GameMessage::CreateSVActionDeath(builder,
                                               this->GetUID(),
                                               0);
    auto msg = GameMessage::CreateMessage(builder,
                                          0,
                                          GameMessage::Messages_SVActionDeath,
                                          move.Union());
    builder.Finish(msg);
    _world._outputMessages.push_back(std::make_shared<MessageBuffer>(builder.GetCurrentBufferPointer(),
                                                                     builder.GetBufferPointer() + builder.GetSize()));
    
    _state = Unit::State::DEAD;
    _objAttributes = GameObject::Attributes::PASSABLE;
    _unitAttributes = 0;
    _health = 0;
}
