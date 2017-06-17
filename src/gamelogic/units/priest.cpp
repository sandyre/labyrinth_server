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
    
    m_nRegenInterval = 2s;
    m_nRegenTimer = 0s;
    m_nRegenAmount = 1;
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
        m_nRegenTimer -= delta;
        if(m_nRegenTimer < 0s &&
           _health <= _maxHealth)
        {
            _health += m_nRegenAmount;
            if(_health > _maxHealth)
                _health = _maxHealth;
            m_nRegenTimer = m_nRegenInterval;
        }
    }
}
