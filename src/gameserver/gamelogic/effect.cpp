//
//  effect.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 12.05.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "effect.hpp"

#include "units/warrior.hpp"

using namespace std::chrono_literals;


Effect::Effect()
: _timer(),
  _state(Effect::State::ACTIVE),
  _name("Default effect")
{
    
}


WarriorDash::WarriorDash(std::chrono::microseconds duration,
                         float bonus_movespeed)
: _bonusSpeed(bonus_movespeed)
{
    _timer = duration;
    _name = "WarriorDash";
}


void
WarriorDash::start()
{
    _targetUnit->_moveSpeed -= _bonusSpeed;
}


void
WarriorDash::update(std::chrono::microseconds delta)
{
    if(_state == Effect::State::ACTIVE)
    {
        _timer -= delta;
        if(_timer < 0s)
        {
            _state = Effect::State::OVER;
        }
    }
}


void
WarriorDash::stop()
{
    _targetUnit->_moveSpeed += _bonusSpeed;
}


WarriorArmorUp::WarriorArmorUp(std::chrono::microseconds duration,
                               int16_t bonus_armor)
: _bonusArmor(bonus_armor)
{
    _timer = duration;
    _name = "WarriorArmorUp";
}


void
WarriorArmorUp::start()
{
    _targetUnit->_armor += _bonusArmor;
}


void
WarriorArmorUp::update(std::chrono::microseconds delta)
{
    if(_state == Effect::State::ACTIVE)
    {
        _timer -= delta;
        if(_timer < 0s)
        {
            _state = Effect::State::OVER;
        }
    }
}


void
WarriorArmorUp::stop()
{
    _targetUnit->_armor -= _bonusArmor;
}


RogueInvisibility::RogueInvisibility(std::chrono::microseconds duration)
{
    _timer = duration;
}


void
RogueInvisibility::start()
{
    _targetUnit->_objAttributes &= ~(GameObject::Attributes::VISIBLE);
    _targetUnit->_unitAttributes &= ~(Unit::Attributes::DUELABLE);
}


void
RogueInvisibility::update(std::chrono::microseconds delta)
{
    if(_state == Effect::State::ACTIVE)
    {
        _timer -= delta;
        if(_timer < 0s)
        {
            _state = Effect::State::OVER;
        }
    }
}


void
RogueInvisibility::stop()
{
    _targetUnit->_objAttributes |= (GameObject::Attributes::VISIBLE);
    _targetUnit->_unitAttributes |= (Unit::Attributes::DUELABLE);
}


MageFreeze::MageFreeze(std::chrono::microseconds duration)
{
    _timer = duration;
    _name = "MageFreeze";
}


void
MageFreeze::start()
{
    _targetUnit->_unitAttributes &= ~(Unit::Attributes::INPUT);
}


void
MageFreeze::update(std::chrono::microseconds delta)
{
    if(_state == Effect::State::ACTIVE)
    {
        _timer -= delta;
        if(_timer < 0s)
        {
            _state = Effect::State::OVER;
        }
    }
}


void
MageFreeze::stop()
{
    _targetUnit->_unitAttributes |= Unit::Attributes::INPUT;
}


DuelInvulnerability::DuelInvulnerability(std::chrono::microseconds duration)
{
    _timer = duration;
    _name = "DuelInvulnerability";
}


void
DuelInvulnerability::start()
{
    _targetUnit->_unitAttributes &= ~(Unit::Attributes::DUELABLE);
}


void
DuelInvulnerability::update(std::chrono::microseconds delta)
{
    if(_state == Effect::State::ACTIVE)
    {
        _timer -= delta;
        if(_timer < 0s)
        {
            _state = Effect::State::OVER;
        }
    }
}


void
DuelInvulnerability::stop()
{
    _targetUnit->_unitAttributes |= Unit::Attributes::DUELABLE;
}


RespawnInvulnerability::RespawnInvulnerability(std::chrono::microseconds duration)
{
    _timer = duration;
    _name = "RespawnInvulnerability";
}


void
RespawnInvulnerability::start()
{
    _targetUnit->_unitAttributes &= ~(Unit::Attributes::DUELABLE);
    _targetUnit->_objAttributes &= ~(GameObject::Attributes::PASSABLE);
}


void
RespawnInvulnerability::update(std::chrono::microseconds delta)
{
    if(_state == Effect::State::ACTIVE)
    {
        _timer -= delta;
        if(_timer < 0s)
        {
            _state = Effect::State::OVER;
        }
    }
}


void
RespawnInvulnerability::stop()
{
    _targetUnit->_unitAttributes |= Unit::Attributes::DUELABLE;
    _targetUnit->_objAttributes |= ~(GameObject::Attributes::PASSABLE);
}


FountainHeal::FountainHeal(std::chrono::microseconds duration)
{
    _timer = duration;
    _name = "FountainHeal";
}


void
FountainHeal::start()
{
    _targetUnit->_health += (_targetUnit->_health.Max() - _targetUnit->_health) / 2;
}


void
FountainHeal::update(std::chrono::microseconds delta)
{
    if (_state == Effect::State::ACTIVE)
    {
        _timer -= delta;
        if (_timer < 0s)
            _state = Effect::State::OVER;
    }
}


void
FountainHeal::stop()
{
    _targetUnit->_health += (_targetUnit->_health.Max() - _targetUnit->_health);
}
