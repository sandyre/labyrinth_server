//
//  monster.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "monster.hpp"

#include "../../gsnet_generated.h"
#include "../gameworld.hpp"

#include <chrono>
using namespace std::chrono_literals;

Monster::Monster()
{
    m_eUnitType = Unit::Type::MONSTER;
    m_sName = "Default monster";
    
    m_nBaseDamage = m_nActualDamage = 10;
    m_nMHealth = m_nHealth = 50;
    
    m_msAtkCD = 3s;
    m_msAtkACD = 0s;
    
    m_msMoveCD = 2s;
    m_msMoveACD = 0s;
}

void
Monster::update(std::chrono::milliseconds delta)
{
        // TODO: add moving ability
    if(m_eState == Unit::State::DUEL)
    {
        if(m_msAtkACD > 0s)
            m_msAtkACD -= delta;
        
        if((m_pDuelTarget->GetAttributes() & GameObject::Attributes::DAMAGABLE) &&
           m_msAtkACD <= 0s)
        {
            m_msAtkACD = m_msAtkCD;
            this->Attack();
        }
    }
}

void
Monster::Spawn(Point2 log_pos)
{
    m_eState = Unit::State::WALKING;
    m_nAttributes = GameObject::Attributes::MOVABLE |
    GameObject::Attributes::VISIBLE |
    GameObject::Attributes::DUELABLE |
    GameObject::Attributes::DAMAGABLE;
    m_nHealth = m_nMHealth;
    
    m_stLogPosition = log_pos;
    
    flatbuffers::FlatBufferBuilder builder;
    auto spawn = GameEvent::CreateSVSpawnMonster(builder,
                                                 this->GetUID(),
                                                 log_pos.x,
                                                 log_pos.y);
    auto msg = GameEvent::CreateMessage(builder,
                                        GameEvent::Events_SVSpawnMonster,
                                        spawn.Union());
    builder.Finish(msg);
    m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
}
