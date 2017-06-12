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

Monster::Monster() :
m_pChasingUnit(nullptr)
{
    m_eUnitType = Unit::Type::MONSTER;
    m_sName = "Skeleton";
    
    m_nBaseDamage = m_nActualDamage = 10;
    m_nMHealth = m_nHealth = 50;
    m_nArmor = 2;
    m_nMResistance = 2;
    
        // spell 1 cd
    m_aSpellCDs.push_back(std::make_tuple(false, 3s, 3s));
    
        // spell 1 seq
    InputSequence atk_seq(5);
    m_aCastSequences.push_back(atk_seq);
    
    m_msCastTime = 600ms;
    m_msACastTime = 0ms;
    
    m_msMoveCD = 2s;
    m_msMoveACD = 0s;
}

void
Monster::update(std::chrono::microseconds delta)
{
    Unit::update(delta);
    
    m_msACastTime -= delta;
    if(m_msACastTime < 0ms)
        m_msACastTime = 0ms;
    
//        // find target to chase
//    if(m_pChasingUnit == nullptr)
//    {
//        for(auto obj : m_poGameWorld->m_apoObjects)
//        {
//            if(obj->GetObjType() == GameObject::Type::UNIT &&
//               obj->GetUID() != this->GetUID() &&
//               Distance(obj->GetLogicalPosition(), this->GetLogicalPosition()) <= 4.0 &&
//               dynamic_cast<Unit*>(obj)->GetUnitAttributes() & Unit::Attributes::DUELABLE)
//            {
//                m_pChasingUnit = dynamic_cast<Unit*>(obj);
//                break;
//            }
//        }
//    }
    if(!(m_nUnitAttributes & Unit::Attributes::INPUT))
        return;
    
    switch (m_eState)
    {
        case Unit::State::DUEL:
        {
            if(m_msACastTime == 0ms)
            {
                m_aCastSequences[0].sequence.pop_back();
                m_msACastTime = m_msCastTime;
                
                if(m_aCastSequences[0].sequence.empty())
                {
                    if(m_pDuelTarget == nullptr)
                        return;
                    
                        // Log damage event
                    auto m_pLogSystem = m_poGameWorld->m_pLogSystem;
                    auto& m_oLogBuilder = m_poGameWorld->m_oLogBuilder;
                    m_oLogBuilder << this->GetName() << " " << m_nActualDamage << " PHYS DMG TO " << m_pDuelTarget->GetName();
                    m_pLogSystem->Info(m_oLogBuilder.str());
                    m_oLogBuilder.str("");
                    
                        // set up CD
                    std::get<0>(m_aSpellCDs[0]) = false;
                    std::get<1>(m_aSpellCDs[0]) = std::get<2>(m_aSpellCDs[0]);
                    
                    flatbuffers::FlatBufferBuilder builder;
                    auto spell_info = GameEvent::CreateMonsterAttack(builder,
                                                                     m_pDuelTarget->GetUID(),
                                                                     m_nActualDamage);
                    auto spell = GameEvent::CreateSpell(builder,
                                                        GameEvent::Spells_MonsterAttack,
                                                        spell_info.Union());
                    auto spell1 = GameEvent::CreateSVActionSpell(builder,
                                                                 this->GetUID(),
                                                                 0,
                                                                 spell);
                    auto event = GameEvent::CreateMessage(builder,
                                                          GameEvent::Events_SVActionSpell,
                                                          spell1.Union());
                    builder.Finish(event);
                    
                    m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                                        builder.GetBufferPointer() + builder.GetSize());
                    
                        // deal PHYSICAL damage
                    m_pDuelTarget->TakeDamage(m_nActualDamage,
                                              Unit::DamageType::PHYSICAL,
                                              this);
                    
                    m_aCastSequences[0].Refresh();
                }
            }
            
            break;
        }
            
        default:
            break;
    }
}

void
Monster::Spawn(Point2 log_pos)
{
    m_eState = Unit::State::WALKING;
    m_nObjAttributes = GameObject::Attributes::MOVABLE |
    GameObject::Attributes::VISIBLE |
    GameObject::Attributes::DAMAGABLE;
    m_nUnitAttributes = Unit::Attributes::INPUT |
    Unit::Attributes::ATTACK |
    Unit::Attributes::DUELABLE;
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

void
Monster::Die(Unit * killer)
{
    if(killer->GetDuelTarget() == this)
    {
        killer->EndDuel();
    }
        // Log dead event
    auto m_pLogSystem = m_poGameWorld->m_pLogSystem;
    auto& m_oLogBuilder = m_poGameWorld->m_oLogBuilder;
    m_oLogBuilder << this->GetName() << " KILLED BY " << killer->GetName() << " DIED AT (" << m_stLogPosition.x << ";" << m_stLogPosition.y << ")";
    m_pLogSystem->Info(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
        // drop items
    while(!m_aInventory.empty())
    {
        this->DropItem(0);
    }
    
    if(m_pDuelTarget != nullptr)
        EndDuel();
    
    flatbuffers::FlatBufferBuilder builder;
    auto move = GameEvent::CreateSVActionDeath(builder,
                                               this->GetUID(),
                                               killer->GetUID());
    auto msg = GameEvent::CreateMessage(builder,
                                        GameEvent::Events_SVActionDeath,
                                        move.Union());
    builder.Finish(msg);
    m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
    
    m_eState = Unit::State::DEAD;
    m_nObjAttributes = GameObject::Attributes::PASSABLE;
    m_nUnitAttributes = 0;
    m_nHealth = 0;
}
