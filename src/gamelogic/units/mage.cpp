//
//  mage.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "mage.hpp"

#include "../gameworld.hpp"
#include "gsnet_generated.h"

#include "../effect.hpp"

#include <chrono>
using namespace std::chrono_literals;

Mage::Mage()
{
    m_eHero = Hero::Type::MAGE;
    m_nMoveSpeed = 0.3;
    m_nMHealth = m_nHealth = 50;
    m_nBaseDamage = m_nActualDamage = 18;
    m_nArmor = 2;
    m_nMResistance = 6;
    
        // spell 1 cd
    m_aSpellCDs.push_back(std::make_tuple(true, 0s, 30s));
    
        // spell 2 cd
    m_aSpellCDs.push_back(std::make_tuple(true, 0s, 0s));
    
        // spell 3 cd
    m_aSpellCDs.push_back(std::make_tuple(true, 0s, 10s));
}

void
Mage::SpellCast(const GameEvent::CLActionSpell* spell)
{
        // teleport cast (0 spell)
    if(spell->spell_id() == 0 &&
       std::get<0>(m_aSpellCDs[0]) == true)
    {
            // set up CD
        std::get<0>(m_aSpellCDs[0]) = false;
        std::get<1>(m_aSpellCDs[0]) = std::get<2>(m_aSpellCDs[0]);
        
        Point2 new_pos;
        while(Distance(this->GetLogicalPosition(), new_pos = m_poGameWorld->GetRandomPosition()) > 10.0)
        {
        }
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell_info = GameEvent::CreateMageTeleport(builder,
                                                        new_pos.x,
                                                        new_pos.y);
        auto spell = GameEvent::CreateSpell(builder,
                                            GameEvent::Spells_MageTeleport,
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
        
        this->SetLogicalPosition(new_pos);
    }
        // attack cast (1 spell)
    else if(spell->spell_id() == 1 &&
            std::get<0>(m_aSpellCDs[1]) == true)
    {
        if(m_pDuelTarget == nullptr)
            return;
        
            // Log damage event
        auto m_pLogSystem = m_poGameWorld->m_pLogSystem;
        auto& m_oLogBuilder = m_poGameWorld->m_oLogBuilder;
        m_oLogBuilder << this->GetName() << " " << m_nActualDamage << " MAG DMG TO " << m_pDuelTarget->GetName();
        m_pLogSystem->Info(m_oLogBuilder.str());
        m_oLogBuilder.str("");
        
            // set up CD
        std::get<0>(m_aSpellCDs[1]) = false;
        std::get<1>(m_aSpellCDs[1]) = std::get<2>(m_aSpellCDs[1]);
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell_info = GameEvent::CreateMageAttack(builder,
                                                      m_pDuelTarget->GetUID(),
                                                      m_nActualDamage);
        auto spell = GameEvent::CreateSpell(builder,
                                            GameEvent::Spells_MageAttack,
                                            spell_info.Union());
        auto spell1 = GameEvent::CreateSVActionSpell(builder,
                                                     this->GetUID(),
                                                     1,
                                                     spell);
        auto event = GameEvent::CreateMessage(builder,
                                              GameEvent::Events_SVActionSpell,
                                              spell1.Union());
        builder.Finish(event);
        
        m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                            builder.GetBufferPointer() + builder.GetSize());
        
            // deal MAGIC damage
        m_pDuelTarget->TakeDamage(m_nActualDamage,
                                  Unit::DamageType::MAGICAL,
                                  this);
    }
        // frostbolt casted (2 spell)
    else if(spell->spell_id() == 2 &&
            std::get<0>(m_aSpellCDs[2]) == true)
    {
        if(m_pDuelTarget == nullptr)
            return;
        
            // set up CD
        std::get<0>(m_aSpellCDs[2]) = false;
        std::get<1>(m_aSpellCDs[2]) = std::get<2>(m_aSpellCDs[2]);
        
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell1 = GameEvent::CreateMageFreeze(builder,
                                                  m_pDuelTarget->GetUID());
        auto spell = GameEvent::CreateSpell(builder,
                                            GameEvent::Spells_MageFreeze,
                                            spell1.Union());
        auto cl_spell = GameEvent::CreateSVActionSpell(builder,
                                                       this->GetUID(),
                                                       2,
                                                       spell);
        auto event = GameEvent::CreateMessage(builder,
                                              GameEvent::Events_SVActionSpell,
                                              cl_spell.Union());
        builder.Finish(event);
        
        m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                            builder.GetBufferPointer() + builder.GetSize());
        
            // apply freeze effect
        MageFreeze * pFreeze = new MageFreeze(3s);
        pFreeze->SetTargetUnit(m_pDuelTarget);
        m_pDuelTarget->ApplyEffect(pFreeze);
    }
}

void
Mage::update(std::chrono::microseconds delta)
{
    Hero::update(delta);
}
