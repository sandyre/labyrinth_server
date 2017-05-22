//
//  hero.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "hero.hpp"

#include "../../gsnet_generated.h"

#include <chrono>
using namespace std::chrono_literals;

Hero::Hero() :
m_eHero(Hero::Type::FIRST_HERO)
{
    m_eUnitType = Unit::Type::HERO;
}

Hero::Type
Hero::GetHero() const
{
    return m_eHero;
}

void
Hero::update(std::chrono::milliseconds delta)
{
    Unit::update(delta);
    UpdateCDs(delta);
}

void
Hero::UpdateCDs(std::chrono::milliseconds delta)
{
    for(int i = 0; i < m_aSpellCDs.size(); ++i)
    {
        if(std::get<0>(m_aSpellCDs[i]) == false)
        {
            std::get<1>(m_aSpellCDs[i]) -= delta;
        }
        else
        {
            std::get<0>(m_aSpellCDs[i]) = true;
            std::get<1>(m_aSpellCDs[i]) = 0s;
        }
    }
}
