//
//  priest.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "priest.hpp"

#include "../../gsnet_generated.h"

#include <chrono>
using namespace std::chrono_literals;

Priest::Priest()
{
    _heroType = Hero::Type::PRIEST;
    _moveSpeed = 0.1;
    _maxHealth = _health = 60;
    _baseDamage = _actualDamage = 10;
    _armor = 4;
    
        // spell 1 cd
    _spellsCDs.push_back(std::make_tuple(true, 0s, 10s));
    
        // spell 2 cd
    _spellsCDs.push_back(std::make_tuple(true, 0s, 10s));
    
    _regenInterval = 2s;
    _regenTimer = 0s;
    _regenAmount = 1;
}

void
Priest::SpellCast(const GameEvent::CLActionSpell*)
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
        _regenTimer -= delta;
        if(_regenTimer < 0s &&
           _health <= _maxHealth)
        {
            _health += _regenAmount;
            if(_health > _maxHealth)
                _health = _maxHealth;
            _regenTimer = _regenInterval;
        }
    }
}
