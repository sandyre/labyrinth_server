//
//  rogue.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "rogue.hpp"

#include "../gameworld.hpp"
#include "../../gsnet_generated.h"
#include "../effect.hpp"

#include <chrono>
using namespace std::chrono_literals;
using Attributes = GameObject::Attributes;

Rogue::Rogue()
{
    m_eHero = Hero::Type::ROGUE;
    m_nMoveSpeed = 0.1;
    m_nMHealth = m_nHealth = 75;
    m_nBaseDamage = m_nActualDamage = 8;
    m_nArmor = 2;
    
        // spell 1 cd
    m_aSpellCDs.push_back(std::make_tuple(true, 0s, 30s));
    
        // spell 2 cd
    m_aSpellCDs.push_back(std::make_tuple(true, 0s, 15s));
}

void
Rogue::SpellCast(const GameEvent::CLActionSpell* spell)
{
        // invisibility cast (0 spell)
    if(spell->spell_id() == 0 &&
       std::get<0>(m_aSpellCDs[0]) == true)
    {
            // set up CD
        std::get<0>(m_aSpellCDs[0]) = false;
        std::get<1>(m_aSpellCDs[0]) = std::get<2>(m_aSpellCDs[0]);
        
        RogueInvisibility * pInvis = new RogueInvisibility(5s);
        pInvis->SetTargetUnit(this);
        this->ApplyEffect(pInvis);
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell1 = GameEvent::CreateSVActionSpell(builder,
                                                     this->GetUID(),
                                                     0);
        auto event = GameEvent::CreateMessage(builder,
                                              GameEvent::Events_SVActionSpell,
                                              spell1.Union());
        builder.Finish(event);
        
        m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                            builder.GetBufferPointer() + builder.GetSize());
    }
        // missing knife cast (1 spell)
    else if(spell->spell_id() == 1)
    {
        
    }
}

void
Rogue::TakeItem(Item * item)
{
    Unit::TakeItem(item);
}

void
Rogue::Attack(const GameEvent::CLActionAttack* atk_msg)
{
    bool bIgnorArmor = false;
        // calculate chance of ignoring the armor
    if(m_oRandDistr(m_oRandGen) % 4 == 0)
    {
        bIgnorArmor = true;
    }
    
    auto m_pLogSystem = m_poGameWorld->m_pLogSystem;
    auto& m_oLogBuilder = m_poGameWorld->m_oLogBuilder;
        // Log duel start event
    m_oLogBuilder << this->GetName() << " ATK TO " << m_pDuelTarget->GetName()
    << " for " << (bIgnorArmor ? m_nActualDamage : m_nActualDamage - m_pDuelTarget->GetArmor());
    m_pLogSystem->Write(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
    m_pDuelTarget->TakeDamage(m_nActualDamage, bIgnorArmor);
    
    flatbuffers::FlatBufferBuilder builder;
    auto atk = GameEvent::CreateSVActionAttack(builder,
                                               this->GetUID(),
                                               m_pDuelTarget->GetUID(),
                                               m_nActualDamage,
                                               bIgnorArmor ? 1 : 0); // FIXME: 1 means 'ignor armor'
    auto msg = GameEvent::CreateMessage(builder,
                                        GameEvent::Events_SVActionAttack,
                                        atk.Union());
    builder.Finish(msg);
    m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
    builder.Clear();
    
    if(m_pDuelTarget->GetState() == Unit::State::DEAD)
    {
            // Log duel end (kill)
        m_oLogBuilder << this->GetName() << " DUEL KILL " << m_pDuelTarget->GetName();
        m_pLogSystem->Write(m_oLogBuilder.str());
        m_oLogBuilder.str("");
        auto kill = GameEvent::CreateSVActionDuel(builder,
                                                  this->GetUID(),
                                                  m_pDuelTarget->GetUID(),
                                                  GameEvent::ActionDuelType_KILL);
        auto msg = GameEvent::CreateMessage(builder,
                                            GameEvent::Events_SVActionDuel,
                                            kill.Union());
        builder.Finish(msg);
        m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                            builder.GetBufferPointer() + builder.GetSize());
        builder.Clear();
        EndDuel();
    }
}

void
Rogue::update(std::chrono::milliseconds delta)
{
    Hero::update(delta);
}
