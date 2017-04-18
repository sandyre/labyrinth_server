//
//  fire_elementalist.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "fire_elementalist.hpp"

#include "gsnet_generated.h"

#include <chrono>
using namespace std::chrono_literals;

FireElementalist::FireElementalist()
{
    m_eHero = Hero::Type::FIRE_ELEMENTALIST;
    m_nMoveSpeed = 0.1;
    m_nMHealth = m_nHealth = 60;
    m_nBaseDamage = m_nActualDamage = 10;
    
    m_nSpell1CD = 30s;
    m_nSpell1ACD = 0s;
}

void
FireElementalist::SpellCast1()
{
        // does nothing
}
