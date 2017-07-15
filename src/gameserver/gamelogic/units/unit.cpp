//
//  unit.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "unit.hpp"

#include "../gameworld.hpp"
#include "../../GameMessage.h"
#include "../effect.hpp"

#include <chrono>
using namespace std::chrono_literals;

using Attributes = GameObject::Attributes;

Unit::Unit(GameWorld& world)
: GameObject(world),
  _unitType(Unit::Type::UNDEFINED),
  _state(Unit::State::UNDEFINED),
  _orientation(Unit::Orientation::DOWN),
  _name("Unit"),
  _baseDamage(10),
  _actualDamage(10),
  _health(50),
  _maxHealth(50),
  _armor(2),
  _magResistance(2),
  _moveSpeed(0.5),
  _duelTarget(nullptr)
{
    _objType = GameObject::Type::UNIT;
    _objAttributes |= GameObject::Attributes::DAMAGABLE;
    _objAttributes |= GameObject::Attributes::MOVABLE;
    _unitAttributes = Unit::Attributes::INPUT |
    Unit::Attributes::ATTACK |
    Unit::Attributes::DUELABLE;
}

void
Unit::ApplyEffect(std::shared_ptr<Effect> effect)
{
        // Log item drop event
    _world._logger.Info() << effect->GetName() << " effect is applied to " << this->GetName();
    _appliedEffects.push_back(effect);
}

void
Unit::update(std::chrono::microseconds delta)
{
    UpdateCDs(delta);
    UpdateStats();
    for(auto effect : _appliedEffects)
    {
        effect->update(delta);
        if(effect->GetState() == Effect::State::OVER)
        {
                // Log item drop event
            _world._logger.Info() << effect->GetName() << " effect ended on " << this->GetName();
            effect->stop();
        }
    }
    _appliedEffects.erase(std::remove_if(_appliedEffects.begin(),
                                         _appliedEffects.end(),
                                         [this](std::shared_ptr<Effect>& eff)
                                         {
                                             return eff->GetState() == Effect::State::OVER;
                                         }),
                          _appliedEffects.end());
}

void
Unit::UpdateCDs(std::chrono::microseconds delta)
{
    for(auto& cd : _spellsCDs)
    {
        if(std::get<0>(cd) == false)
        {
            std::get<1>(cd) -= delta;
            
            if(std::get<1>(cd) <= 0s)
            {
                std::get<0>(cd) = true;
                std::get<1>(cd) = 0s;
            }
        }
    }
}

void
Unit::UpdateStats()
{
    int new_dmg = _baseDamage;
    for(auto item : _inventory)
    {
        if(item->GetType() == Item::Type::SWORD)
            new_dmg += 6;
    }
    _actualDamage = new_dmg;
}

void
Unit::TakeItem(std::shared_ptr<Item> item)
{
        // Log item drop event
    _world._logger.Info() << this->GetName() << " TOOK ITEM " << (int)item->GetType();
    _inventory.push_back(item);
    
    flatbuffers::FlatBufferBuilder builder;
    auto take = GameMessage::CreateSVActionItem(builder,
                                              this->GetUID(),
                                              item->GetUID(),
                                              GameMessage::ActionItemType_TAKE);
    auto msg = GameMessage::CreateMessage(builder,
                                        0,
                                        GameMessage::Messages_SVActionItem,
                                        take.Union());
    builder.Finish(msg);
    _world._outputEvents.emplace(builder.GetBufferPointer(),
                                 builder.GetBufferPointer() + builder.GetSize());
}

void
Unit::Spawn(Point<> log_pos)
{
        // Log spawn event
    _world._logger.Info() << this->GetName() << " SPWN AT (" << log_pos.x << ";" << log_pos.y << ")";
    
    _state = Unit::State::WALKING;
    _objAttributes = GameObject::Attributes::MOVABLE |
                        GameObject::Attributes::VISIBLE |
                        GameObject::Attributes::DAMAGABLE;
    _unitAttributes = Unit::Attributes::INPUT |
    Unit::Attributes::ATTACK |
    Unit::Attributes::DUELABLE;
    _health = _maxHealth;
    
    _pos = log_pos;
    
    flatbuffers::FlatBufferBuilder builder;
    auto spawn = GameMessage::CreateSVSpawnPlayer(builder,
                                                this->GetUID(),
                                                log_pos.x,
                                                log_pos.y);
    auto msg = GameMessage::CreateMessage(builder,
                                        0,
                                        GameMessage::Messages_SVSpawnPlayer,
                                        spawn.Union());
    builder.Finish(msg);
    _world._outputEvents.emplace(builder.GetBufferPointer(),
                                 builder.GetBufferPointer() + builder.GetSize());
}

void
Unit::Respawn(Point<> log_pos)
{
        // Log respawn event
    _world._logger.Info() << this->GetName() << " RESP AT (" << log_pos.x << ";" << log_pos.y << ")";
    
    _state = Unit::State::WALKING;
    _objAttributes = GameObject::Attributes::MOVABLE |
    GameObject::Attributes::VISIBLE |
    GameObject::Attributes::DAMAGABLE;
    _unitAttributes = Unit::Attributes::INPUT |
    Unit::Attributes::ATTACK |
    Unit::Attributes::DUELABLE;
    _health = _maxHealth;
    
    _pos = log_pos;

    auto respBuff = std::make_shared<RespawnInvulnerability>(5s);
    respBuff->SetTargetUnit(std::static_pointer_cast<Unit>(shared_from_this()));
    this->ApplyEffect(respBuff);
    
    flatbuffers::FlatBufferBuilder builder;
    auto resp = GameMessage::CreateSVRespawnPlayer(builder,
                                                 this->GetUID(),
                                                 log_pos.x,
                                                 log_pos.y);
    auto msg = GameMessage::CreateMessage(builder,
                                        0,
                                        GameMessage::Messages_SVRespawnPlayer,
                                        resp.Union());
    builder.Finish(msg);
    _world._outputEvents.emplace(builder.GetBufferPointer(),
                                 builder.GetBufferPointer() + builder.GetSize());
}

std::shared_ptr<Item>
Unit::DropItem(int32_t uid)
{
    auto item = std::find_if(_inventory.begin(),
                             _inventory.end(),
                             [uid](const std::shared_ptr<Item>& item)
                             {
                                 return item->GetUID() == uid;
                             });

    if(item == _inventory.end())
    {
        _world._logger.Error() << "Failed to drop item with uid" << uid << ". Unit does not handle it.";
        return nullptr;
    }
    
        // Log item drop event
    _world._logger.Info() << _name << " DROPPED ITEM " << (int)(*item)->GetType();
    (*item)->SetPosition(_pos);

    auto item_ptr = *item;
    _inventory.erase(item);
    return item_ptr;
}

void
Unit::Die(const std::string& killerName)
{
        // Log death event
    _world._logger.Info() << this->GetName() << " KILLED BY " << killerName << " DIED AT (" << _pos.x << ";" << _pos.y << ")";
    
        // drop items
    while(!_inventory.empty())
    {
        this->DropItem(0);
    }
    
    if(_duelTarget != nullptr)
        EndDuel();
    
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
    _world._respawnQueue.push_back(std::make_pair(3s, this));
}

void
Unit::Move(MoveDirection dir)
{
    Point<> new_coord = _pos;
    if(dir == MoveDirection::LEFT)
        --new_coord.x;
    else if(dir == MoveDirection::RIGHT)
        ++new_coord.x;
    else if(dir == MoveDirection::UP)
        ++new_coord.y;
    else if(dir == MoveDirection::DOWN)
        --new_coord.y;
    
        // firstly - check if it can go there (no unpassable objects)
    for(auto object : _world._objects)
    {
        if(object->GetPosition() == new_coord &&
           !(object->GetAttributes() & GameObject::Attributes::PASSABLE))
        {
            return; // there is an unpassable object
        }
    }
        // Log move event
    _world._logger.Info() << this->GetName() << " MOVE (" << _pos.x
    << ";" << _pos.y << ") -> (" << new_coord.x << ";" << new_coord.y << ")";

    _pos = new_coord;
    
    flatbuffers::FlatBufferBuilder builder;
    auto move = GameMessage::CreateSVActionMove(builder,
                                              this->GetUID(),
                                              (char)dir,
                                              new_coord.x,
                                              new_coord.y);
    auto msg = GameMessage::CreateMessage(builder,
                                        0,
                                        GameMessage::Messages_SVActionMove,
                                        move.Union());
    builder.Finish(msg);
    _world._outputEvents.emplace(builder.GetBufferPointer(),
                                 builder.GetBufferPointer() + builder.GetSize());
}

void
Unit::TakeDamage(const DamageDescriptor& dmg)
{
    int16_t damage_taken = dmg.Value;

    if(dmg.Type == DamageDescriptor::DamageType::PHYSICAL)
        damage_taken -= _armor;
    else if(dmg.Type == DamageDescriptor::DamageType::MAGICAL)
        damage_taken -= _magResistance;
    
    _health -= damage_taken;
    
        // Log damage take event
    _world._logger.Info() << this->GetName() << " TOOK " << damage_taken << " FROM " << dmg.DealerName << " HP " << _health + damage_taken << "->" << _health;
    
    if(_health <= 0)
    {
        Die(dmg.DealerName);
    }
}

void
Unit::StartDuel(std::shared_ptr<Unit> enemy)
{
        // FIXME: each duel sends 2 msges about duel start.
        // client now can eat it, but server should also be reworked
    
        // Log duel start event
    _world._logger.Info() << "DUEL START  " << this->GetName() << " and " << enemy->GetName();
    
    _state = Unit::State::DUEL;
    _unitAttributes &= ~Unit::Attributes::DUELABLE;
    
    _duelTarget = enemy;
    
    flatbuffers::FlatBufferBuilder builder;
    auto duel = GameMessage::CreateSVActionDuel(builder,
                                              this->GetUID(),
                                              enemy->GetUID(),
                                              GameMessage::ActionDuelType_STARTED);
    auto msg = GameMessage::CreateMessage(builder,
                                        0,
                                        GameMessage::Messages_SVActionDuel,
                                        duel.Union());
    builder.Finish(msg);
    
    _world._outputEvents.emplace(builder.GetBufferPointer(),
                                 builder.GetBufferPointer() + builder.GetSize());
}

void
Unit::EndDuel()
{
        // Log duel-end event
    _world._logger.Info() << this->GetName() << " DUEL END W " << _duelTarget->GetName();
    
    _state = Unit::State::WALKING;
    _unitAttributes |=
    Unit::Attributes::INPUT |
    Unit::Attributes::ATTACK |
    Unit::Attributes::DUELABLE;
    
    _duelTarget = nullptr;
}
