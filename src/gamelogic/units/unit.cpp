//
//  unit.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "unit.hpp"

#include "../gameworld.hpp"
#include "../../gsnet_generated.h"
#include "../effect.hpp"

#include <chrono>
using namespace std::chrono_literals;

using Attributes = GameObject::Attributes;

Unit::Unit() :
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

Unit::Type
Unit::GetUnitType() const
{
    return _unitType;
}

Unit::State
Unit::GetState() const
{
    return _state;
}

Unit::Orientation
Unit::GetOrientation() const
{
    return _orientation;
}

std::string
Unit::GetName() const
{
    return _name;
}

void
Unit::SetName(const std::string& name)
{
    _name = name;
}

int16_t
Unit::GetDamage() const
{
    return _actualDamage;
}

int16_t
Unit::GetHealth() const
{
    return _health;
}

int16_t
Unit::GetMaxHealth() const
{
    return _maxHealth;
}

int16_t
Unit::GetArmor() const
{
    return _armor;
}

Unit * const
Unit::GetDuelTarget() const
{
    return _duelTarget;
}

uint32_t
Unit::GetUnitAttributes() const
{
    return _unitAttributes;
}

std::vector<Item*>&
Unit::GetInventory()
{
    return _inventory;
}

/*
 *
 * Animations
 *
 */

void
Unit::ApplyEffect(Effect * effect)
{
        // Log item drop event
    _gameWorld->_logger.Info() << effect->GetName() << " effect is applied to " << this->GetName();
    _appliedEffects.push_back(effect);
}

void
Unit::update(std::chrono::microseconds delta)
{
    UpdateCDs(delta);
    for(auto effect : _appliedEffects)
    {
        effect->update(delta);
        if(effect->GetState() == Effect::State::OVER)
        {
                // Log item drop event
            _gameWorld->_logger.Info() << effect->GetName() << " effect ended on " << this->GetName();
            effect->stop();
        }
    }
    _appliedEffects.erase(std::remove_if(_appliedEffects.begin(),
                                         _appliedEffects.end(),
                                         [this](Effect * eff)
                                         {
                                             if(eff->GetState() == Effect::State::OVER)
                                             {
                                                 delete eff;
                                                 return true;
                                             }
                                             return false;
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
Unit::TakeItem(Item * item)
{
        // Log item drop event
    _gameWorld->_logger.Info() << this->GetName() << " TOOK ITEM " << (int)item->GetType();
    
    item->SetCarrierID(this->GetUID());
    _inventory.push_back(item);
    
    flatbuffers::FlatBufferBuilder builder;
    auto take = GameEvent::CreateSVActionItem(builder,
                                              this->GetUID(),
                                              item->GetUID(),
                                              GameEvent::ActionItemType_TAKE);
    auto msg = GameEvent::CreateMessage(builder,
                                        0,
                                        GameEvent::Events_SVActionItem,
                                        take.Union());
    builder.Finish(msg);
    _gameWorld->_outputEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
}

void
Unit::Spawn(Point2 log_pos)
{
        // Log spawn event
    _gameWorld->_logger.Info() << this->GetName() << " SPWN AT (" << log_pos.x << ";" << log_pos.y << ")";
    
    _state = Unit::State::WALKING;
    _objAttributes = GameObject::Attributes::MOVABLE |
                        GameObject::Attributes::VISIBLE |
                        GameObject::Attributes::DAMAGABLE;
    _unitAttributes = Unit::Attributes::INPUT |
    Unit::Attributes::ATTACK |
    Unit::Attributes::DUELABLE;
    _health = _maxHealth;
    
    _logPos = log_pos;
    
    flatbuffers::FlatBufferBuilder builder;
    auto spawn = GameEvent::CreateSVSpawnPlayer(builder,
                                                this->GetUID(),
                                                log_pos.x,
                                                log_pos.y);
    auto msg = GameEvent::CreateMessage(builder,
                                        0,
                                        GameEvent::Events_SVSpawnPlayer,
                                        spawn.Union());
    builder.Finish(msg);
    _gameWorld->_outputEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
}

void
Unit::Respawn(Point2 log_pos)
{
        // Log respawn event
    _gameWorld->_logger.Info() << this->GetName() << " RESP AT (" << log_pos.x << ";" << log_pos.y << ")";
    
    _state = Unit::State::WALKING;
    _objAttributes = GameObject::Attributes::MOVABLE |
    GameObject::Attributes::VISIBLE |
    GameObject::Attributes::DAMAGABLE;
    _unitAttributes = Unit::Attributes::INPUT |
    Unit::Attributes::ATTACK |
    Unit::Attributes::DUELABLE;
    _health = _maxHealth;
    
    _logPos = log_pos;
    
    RespawnInvulnerability * pRespInv = new RespawnInvulnerability(5s);
    pRespInv->SetTargetUnit(this);
    this->ApplyEffect(pRespInv);
    
    flatbuffers::FlatBufferBuilder builder;
    auto resp = GameEvent::CreateSVRespawnPlayer(builder,
                                                 this->GetUID(),
                                                 log_pos.x,
                                                 log_pos.y);
    auto msg = GameEvent::CreateMessage(builder,
                                        0,
                                        GameEvent::Events_SVRespawnPlayer,
                                        resp.Union());
    builder.Finish(msg);
    _gameWorld->_outputEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
}

void
Unit::DropItem(int32_t index)
{
    auto item_iter = _inventory.begin() + index;
    
        // Log item drop event
    _gameWorld->_logger.Info() << this->GetName() << " DROPPED ITEM " << (int)(*item_iter)->GetType();
    
        // make item visible and set its coords
    (*item_iter)->SetCarrierID(0);
    (*item_iter)->SetLogicalPosition(this->GetLogicalPosition());
    
        // delete item from inventory
    _inventory.erase(item_iter);
}

void
Unit::Die(Unit * killer)
{
    if(killer->GetDuelTarget() == this)
    {
        killer->EndDuel();
    }
        // Log death event
    _gameWorld->_logger.Info() << this->GetName() << " KILLED BY " << killer->GetName() << " DIED AT (" << _logPos.x << ";" << _logPos.y << ")";
    
        // drop items
    while(!_inventory.empty())
    {
        this->DropItem(0);
    }
    
    if(_duelTarget != nullptr)
        EndDuel();
    
    flatbuffers::FlatBufferBuilder builder;
    auto move = GameEvent::CreateSVActionDeath(builder,
                                               this->GetUID(),
                                               killer->GetUID());
    auto msg = GameEvent::CreateMessage(builder,
                                        0,
                                        GameEvent::Events_SVActionDeath,
                                        move.Union());
    builder.Finish(msg);
    _gameWorld->_outputEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
    
    _state = Unit::State::DEAD;
    _objAttributes = GameObject::Attributes::PASSABLE;
    _unitAttributes = 0;
    _health = 0;
    _gameWorld->_respawnQueue.push_back(std::make_pair(3s, this));
}

void
Unit::Move(MoveDirection dir)
{
    Point2 new_coord = this->GetLogicalPosition();
    if(dir == MoveDirection::LEFT)
        --new_coord.x;
    else if(dir == MoveDirection::RIGHT)
        ++new_coord.x;
    else if(dir == MoveDirection::UP)
        ++new_coord.y;
    else if(dir == MoveDirection::DOWN)
        --new_coord.y;
    
        // firstly - check if it can go there (no unpassable objects)
    for(auto object : _gameWorld->_objects)
    {
        if(object->GetLogicalPosition() == new_coord &&
           !(object->GetAttributes() & GameObject::Attributes::PASSABLE))
        {
            return; // there is an unpassable object
        }
    }
        // Log move event
    _gameWorld->_logger.Info() << this->GetName() << " MOVE (" << _logPos.x
    << ";" << _logPos.y << ") -> (" << new_coord.x << ";" << new_coord.y << ")";
    
    this->SetLogicalPosition(new_coord);
    
    flatbuffers::FlatBufferBuilder builder;
    auto move = GameEvent::CreateSVActionMove(builder,
                                              this->GetUID(),
                                              (char)dir,
                                              new_coord.x,
                                              new_coord.y);
    auto msg = GameEvent::CreateMessage(builder,
                                        0,
                                        GameEvent::Events_SVActionMove,
                                        move.Union());
    builder.Finish(msg);
    _gameWorld->_outputEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
}

void
Unit::TakeDamage(int16_t damage,
                 Unit::DamageType dmg_type,
                 Unit * damage_dealer)
{
    int16_t damage_taken = damage;
    if(dmg_type == Unit::DamageType::PHYSICAL)
        damage_taken -= _armor;
    else if(dmg_type == Unit::DamageType::MAGICAL)
        damage_taken -= _magResistance;
    
    _health -= damage_taken;
    
        // Log damage take event
    _gameWorld->_logger.Info() << this->GetName() << " TOOK " << damage_taken << " FROM " << damage_dealer->GetName()
    << " HP " << _health + damage_taken << "->" << _health;
    
    if(_health <= 0)
    {
        Die(damage_dealer);
    }
}

void
Unit::StartDuel(Unit * enemy)
{
        // FIXME: each duel sends 2 msges about duel start.
        // client now can eat it, but server should also be reworked
    
        // Log duel start event
    _gameWorld->_logger.Info() << "DUEL START  " << this->GetName() << " and " << enemy->GetName();
    
    _state = Unit::State::DUEL;
    _unitAttributes &= ~Unit::Attributes::DUELABLE;
    
    _duelTarget = enemy;
    
    flatbuffers::FlatBufferBuilder builder;
    auto duel = GameEvent::CreateSVActionDuel(builder,
                                              this->GetUID(),
                                              enemy->GetUID(),
                                              GameEvent::ActionDuelType_STARTED);
    auto msg = GameEvent::CreateMessage(builder,
                                        0,
                                        GameEvent::Events_SVActionDuel,
                                        duel.Union());
    builder.Finish(msg);
    
    _gameWorld->_outputEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
}

void
Unit::EndDuel()
{
        // Log duel-end event
    _gameWorld->_logger.Info() << this->GetName() << " DUEL END W " << _duelTarget->GetName();
    
    _state = Unit::State::WALKING;
    _unitAttributes |=
    Unit::Attributes::INPUT |
    Unit::Attributes::ATTACK |
    Unit::Attributes::DUELABLE;
    
    _duelTarget = nullptr;
}
