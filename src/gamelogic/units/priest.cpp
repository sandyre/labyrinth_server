//
//  priest.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "priest.hpp"

#include "gsnet_generated.h"

#include <chrono>
using namespace std::chrono_literals;

Priest::Priest()
{
    m_eHero = Hero::Type::PRIEST;
    m_nMoveSpeed = 0.1;
    m_nMHealth = m_nHealth = 60;
    m_nBaseDamage = m_nActualDamage = 10;
    m_nArmor = 4;
    
        // spell 1 cd
    m_aSpellCDs.push_back(std::make_tuple(true, 0s, 10s));
    
        // spell 2 cd
    m_aSpellCDs.push_back(std::make_tuple(true, 0s, 10s));
    
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
    if(m_eState == Unit::State::DUEL)
    {
        m_nRegenTimer -= delta;
        if(m_nRegenTimer < 0s &&
           m_nHealth <= m_nMHealth)
        {
            m_nHealth += m_nRegenAmount;
            if(m_nHealth > m_nMHealth)
                m_nHealth = m_nMHealth;
            m_nRegenTimer = m_nRegenInterval;
        }
    }
}
