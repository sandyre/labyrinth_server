//
//  water_elementalist.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "water_elementalist.hpp"

#include "../gameworld.hpp"
#include "gsnet_generated.h"

#include <chrono>
using namespace std::chrono_literals;

WaterElementalist::WaterElementalist()
{
    m_eHero = Hero::Type::WATER_ELEMENTALIST;
    m_nMoveSpeed = 0.3;
    m_nMHealth = m_nHealth = 75;
    m_nBaseDamage = m_nActualDamage = 8;
    
    m_nSpell1CD = 30s;
    m_nSpell1ACD = 0s;
    
    m_nDashDuration = 10s;
    m_nDashADuration = 0s;
    m_bDashing = false;
}

void
WaterElementalist::SpellCast1()
{
    m_nSpell1ACD = m_nSpell1CD;
    m_nDashADuration = m_nDashDuration;
    m_bDashing = true;
    
    m_nMoveSpeed = 0.001;
    
    flatbuffers::FlatBufferBuilder builder;
    auto spell1 = GameEvent::CreateCLActionSpell(builder,
                                                 this->GetUID(),
                                                 1,
                                                 GameEvent::ActionSpellTarget_TARGET_PLAYER,
                                                 this->GetUID());
    auto event = GameEvent::CreateMessage(builder,
                                          GameEvent::Events_CLActionSpell,
                                          spell1.Union());
    builder.Finish(event);
    
    m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
}

void
WaterElementalist::update(std::chrono::milliseconds delta)
{
    Hero::update(delta);

    if(m_bDashing &&
       m_nDashADuration > 0s)
    {
        m_nDashADuration -= delta;
    }
    else if(m_bDashing &&
            m_nDashADuration < 0s)
    {
        m_bDashing = false;
        m_nMoveSpeed = 0.3;
    }
}
