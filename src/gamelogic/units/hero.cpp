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
m_eHero(Hero::Type::FIRST_HERO),
m_nSpell1CD(0s),
m_nSpell1ACD(0s)
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
    UpdateCDs(delta);
}

void
Hero::UpdateCDs(std::chrono::milliseconds delta)
{
    if(m_nSpell1ACD > 0s)
    {
        m_nSpell1ACD -= delta;
    }
    else
    {
        m_nSpell1ACD = 0s;
    }
}

bool
Hero::isSpellCast1Ready() const
{
    if(m_nSpell1ACD == 0s)
        return true;
    return false;
}

std::chrono::milliseconds
Hero::GetSpell1ACD() const
{
    return m_nSpell1ACD;
}
