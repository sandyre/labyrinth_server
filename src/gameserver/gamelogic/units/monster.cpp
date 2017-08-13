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
#include "../../../toolkit/RatMaze.hpp"

#include <chrono>
using namespace std::chrono_literals;


Monster::Monster(GameWorld& world, uint32_t uid)
: Unit(world, uid)
{
    _unitType = Unit::Type::MONSTER;
    _name = "Skeleton";
    
    _baseDamage = _actualDamage = 10;
    _maxHealth = _health = 50;
    _armor = 2;
    _magResistance = 2;
    
        // spell 1 cd
    _cdManager.AddSpell(3s);
    
        // spell 1 seq
    InputSequence atk_seq(5);
    _castSequence.push_back(atk_seq);
    
    _castTime = 600ms;
    _castATime = 0ms;
    
    _moveCD = 1s;
    _moveACD = 0s;
}


void
Monster::update(std::chrono::microseconds delta)
{
    Unit::update(delta);
    
    _castATime -= delta;
    if(_castATime < 0ms)
        _castATime = 0ms;

    _moveACD -= delta;
    if(_moveACD < 0ms)
        _moveACD = 0ms;

    // find target to chase
    if(!_chasingUnit)
    {
        auto units = _world._objectsStorage.Subset<Unit>();
        for(auto unit : units)
        {
            if(unit->GetUID() != this->GetUID() &&
               unit->GetType() != Unit::Type::MONSTER &&
               unit->GetPosition().Distance(this->GetPosition()) <= 7.0)
            {
                _chasingUnit = unit;
                _world._logger.Info() << this->GetName() << " CHASING " << (*_chasingUnit)->GetName();
            }
        }
    }

    if(_chasingUnit) // make a path
    {
        if((*_chasingUnit)->GetPosition().Distance(this->GetPosition()) > 8.0) // check that enemy is still in radius
        {
            _world._logger.Info() << this->GetName() << " IS NO LONGER CHASING " << (*_chasingUnit)->GetName();
            _chasingUnit.reset();
            _pathToUnit.reset();
        }
        else // calculate path?
        {
            if(!_pathToUnit ||
               _pathToUnit->back() != (*_chasingUnit)->GetPosition()) // path exists
            {
                // it means that object moved since last update. recalculation needed
                auto mapSize = _world._mapConf.MapSize * _world._mapConf.RoomSize + 2;
                std::vector<std::vector<uint8_t>> binary_world(mapSize, std::vector<uint8_t>(mapSize, 1));

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

                auto path = RatMaze(binary_world, this->GetPosition(), (*_chasingUnit)->GetPosition());
                if(path) // path found!
                {
                    _pathToUnit = path;
                    _pathToUnit->pop_front();
                }
            }
        }
    }

    if(!(_unitAttributes & Unit::Attributes::INPUT))
        return;
    
    switch (_state)
    {
    case Unit::State::WALKING:
    {
        if(_pathToUnit && _moveACD <= 0ms)
        {
            Point<> nextPos = _pathToUnit->front();

            if(nextPos.x > _pos.x)
                Move(Unit::MoveDirection::RIGHT);
            else if(nextPos.x < _pos.x)
                Move(Unit::MoveDirection::LEFT);
            else if(nextPos.y > _pos.y)
                Move(Unit::MoveDirection::UP);
            else if(nextPos.y < _pos.y)
                Move(Unit::MoveDirection::DOWN);

            if(nextPos == _pos)
            {
                _pathToUnit->pop_front();
                _moveACD = _moveCD;
            }
        }
        else if(_pathToUnit && _pathToUnit->size() == 1)
            this->StartDuel(*_chasingUnit);
        break;
    }

    case Unit::State::DUEL:
    {
        if(_castATime == 0ms)
        {
            _castSequence[0].sequence.pop_back();
            _castATime = _castTime;
            
            if(_castSequence[0].sequence.empty())
            {
                if(_duelTarget == nullptr)
                    return;
                
                    // Log damage event
                _world._logger.Info() << this->GetName() << " " << _actualDamage << " PHYS DMG TO " << _duelTarget->GetName();
                
                    // set up CD
                _cdManager.Restart(0);
                
                flatbuffers::FlatBufferBuilder builder;
                auto spell_info = GameMessage::CreateMonsterAttack(builder,
                                                                 _duelTarget->GetUID(),
                                                                 _actualDamage);
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
                
                _world._outputEvents.emplace(builder.GetBufferPointer(),
                                             builder.GetBufferPointer() + builder.GetSize());
                
                    // deal PHYSICAL damage
                auto dmgDescr = Unit::DamageDescriptor();
                dmgDescr.DealerName = _name;
                dmgDescr.Value = _actualDamage;
                dmgDescr.Type = Unit::DamageDescriptor::DamageType::PHYSICAL;
                _duelTarget->TakeDamage(dmgDescr);
                
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
    _world._logger.Info() << this->GetName() << " SPWN AT (" << pos.x << ";" << pos.y << ")";

    _state = Unit::State::WALKING;
    _objAttributes = GameObject::Attributes::MOVABLE | GameObject::Attributes::VISIBLE | GameObject::Attributes::DAMAGABLE;
    _unitAttributes = Unit::Attributes::INPUT | Unit::Attributes::ATTACK | Unit::Attributes::DUELABLE;
    _health = _maxHealth;
    
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
    _world._outputEvents.emplace(builder.GetBufferPointer(),
                                 builder.GetBufferPointer() + builder.GetSize());
}


void
Monster::Die(const std::string& killerName)
{
        // Log death event
    _world._logger.Info() << this->GetName() << " KILLED BY " << killerName << " DIED AT (" << _pos.x << ";" << _pos.y << ")";
    
    // drop items
    auto items = _inventory;
    for(auto item : items)
        this->DropItem(item->GetUID());

    if(_duelTarget)
    {
        _duelTarget->EndDuel();
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
    _world._outputEvents.emplace(builder.GetBufferPointer(),
                                 builder.GetBufferPointer() + builder.GetSize());
    
    _state = Unit::State::DEAD;
    _objAttributes = GameObject::Attributes::PASSABLE;
    _unitAttributes = 0;
    _health = 0;
}
