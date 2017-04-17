//
//  monster.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "monster.hpp"

Monster::Monster()
{
    m_eUnitType = Unit::Type::MONSTER;
    m_sName = "Default monster";
    
    m_nBaseDamage = m_nActualDamage = 10;
    m_nMHealth = m_nHealth = 50;
}
