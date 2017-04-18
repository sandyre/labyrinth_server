//
//  air_elementalist.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "air_elementalist.hpp"

#include "../gameworld.hpp"
#include "../../gsnet_generated.h"

#include <chrono>
using namespace std::chrono_literals;
using Attributes = GameObject::Attributes;

AirElementalist::AirElementalist()
{
    m_eHero = Hero::Type::AIR_ELEMENTALIST;
    m_nMoveSpeed = 0.1;
    m_nMHealth = m_nHealth = 75;
    m_nBaseDamage = m_nActualDamage = 6;
    
    m_nSpell1CD = 30s;
    m_nSpell1ACD = 0s;
    
    m_nInvisDuration = 10s;
    m_nInvisADuration = 0s;
}

void
AirElementalist::SpellCast1()
{
    m_nSpell1ACD = m_nSpell1CD;
    m_nAttributes ^= Attributes::DUELABLE;
    m_nAttributes ^= Attributes::VISIBLE;
    
    m_nInvisADuration = m_nInvisDuration;
    
    flatbuffers::FlatBufferBuilder builder;
    auto spell1 = GameEvent::CreateSVActionSpell(builder,
                                                 this->GetUID(),
                                                 1,
                                                 GameEvent::ActionSpellTarget_TARGET_PLAYER,
                                                 this->GetUID());
    auto event = GameEvent::CreateMessage(builder,
                                          GameEvent::Events_SVActionSpell,
                                          spell1.Union());
    builder.Finish(event);
    
    m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
}

void
AirElementalist::TakeItem(Item * item)
{
    Unit::TakeItem(item);

    if(!(m_nAttributes & Attributes::VISIBLE))
    {
        m_nInvisADuration = 0s;
        m_nAttributes |= Attributes::DUELABLE;
        m_nAttributes |= Attributes::VISIBLE;
    }
}

void
AirElementalist::update(std::chrono::milliseconds delta)
{
    Hero::update(delta);

    if(!(m_nAttributes & Attributes::VISIBLE) &&
       m_nInvisADuration > 0s)
    {
        m_nInvisADuration -= delta;
    }
    else if(!(m_nAttributes & Attributes::VISIBLE) &&
            m_nInvisADuration < 0s)
    {
            // make visible & duelable
        m_nInvisADuration = 0s;
        m_nAttributes |= Attributes::DUELABLE;
        m_nAttributes |= Attributes::VISIBLE;
    }
}
