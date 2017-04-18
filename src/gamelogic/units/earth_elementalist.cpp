//
//  earth_elementalist.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "earth_elementalist.hpp"

#include "gsnet_generated.h"

#include <chrono>
using namespace std::chrono_literals;

EarthElementalist::EarthElementalist()
{
    m_eHero = Hero::Type::EARTH_ELEMENTALIST;
    m_nMoveSpeed = 0.4;
    m_nMHealth = m_nHealth = 100;
    m_nBaseDamage = m_nActualDamage = 8;
    
    m_nSpell1CD = 3s;
    m_nSpell1ACD = 0s;
}

void
EarthElementalist::SpellCast1()
{
        // no skill right now
}
