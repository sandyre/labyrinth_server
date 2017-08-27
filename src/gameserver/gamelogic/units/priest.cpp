//
//  priest.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "priest.hpp"

#include "../../GameMessage.h"

#include <chrono>
using namespace std::chrono_literals;


Priest::Priest(GameWorld& world, uint32_t uid)
: Hero(world, uid)
{
    _heroType = Hero::Type::PRIEST;
    _moveSpeed = 0.1;
    _health = SimpleProperty<>(60, 0, 60);
    _damage = SimpleProperty<>(10, 0, 100);
    _armor = SimpleProperty<>(4, 0, 100);
    
        // spell 1 cd
    _cdManager.AddSpell(10s);
    
        // spell 2 cd
    _cdManager.AddSpell(10s);
    
    _regenInterval = 2s;
    _regenTimer = 0s;
    _regenAmount = 1;
}


void
Priest::SpellCast(const GameMessage::CLActionSpell*)
{
        // does nothing
}


void
Priest::update(std::chrono::microseconds delta)
{
    Hero::update(delta);
    
        // passive regen works only in duel mode
    if(_state == Unit::State::DUEL)
    {
//        _regenTimer -= delta;
//        if(_regenTimer < 0s &&
//           _health <= _maxHealth)
//        {
//            _health += _regenAmount;
//            if(_health > _maxHealth)
//                _health = _maxHealth;
//            _regenTimer = _regenInterval;
//        }
    }
}
