//
//  warrior.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "warrior.hpp"

#include "../gameworld.hpp"
#include "gsnet_generated.h"
#include "../effect.hpp"

#include <chrono>
using namespace std::chrono_literals;

Warrior::Warrior()
{
    m_eHero = Hero::Type::WARRIOR;
    m_nMoveSpeed = 0.4;
    m_nMHealth = m_nHealth = 80;
    m_nBaseDamage = m_nActualDamage = 10;
    m_nArmor = 6;
    m_nMResistance = 2;
    
        // spell 1 cd
    m_aSpellCDs.push_back(std::make_tuple(true, 0s, 10s));
    
        // spell 2 cd
    m_aSpellCDs.push_back(std::make_tuple(true, 0s, 0s));
    
        // spell 3 cd
    m_aSpellCDs.push_back(std::make_tuple(true, 0s, 10s));
}

void
Warrior::SpellCast(const GameEvent::CLActionSpell* spell)
{
        // warrior dash cast (0 spell)
    if(spell->spell_id() == 0 &&
       std::get<0>(m_aSpellCDs[0]) == true) // check CD
    {
            // set up CD
        std::get<0>(m_aSpellCDs[0]) = false;
        std::get<1>(m_aSpellCDs[0]) = std::get<2>(m_aSpellCDs[0]);
        
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
        
        WarriorDash * pDash = new WarriorDash(3s,
                                              5.5);
        pDash->SetTargetUnit(this);
        this->ApplyEffect(pDash);
    }
    else if(spell->spell_id() == 1 &&
            std::get<0>(m_aSpellCDs[1]) == true) // warrior attack (2 spell)
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
        std::get<0>(m_aSpellCDs[1]) = false;
        std::get<1>(m_aSpellCDs[1]) = std::get<2>(m_aSpellCDs[1]);
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell_info = GameEvent::CreateWarriorAttack(builder,
                                                      m_pDuelTarget->GetUID(),
                                                      m_nActualDamage);
        auto spell = GameEvent::CreateSpell(builder,
                                            GameEvent::Spells_WarriorAttack,
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
        
            // deal PHYSICAL damage
        m_pDuelTarget->TakeDamage(m_nActualDamage,
                                  Unit::DamageType::PHYSICAL,
                                  this);
    }
        // warrior armor up cast (2 spell)
    else if(spell->spell_id() == 2 &&
            std::get<0>(m_aSpellCDs[2]) == true) // check CD
    {
            // set up CD
        std::get<0>(m_aSpellCDs[2]) = false;
        std::get<1>(m_aSpellCDs[2]) = std::get<2>(m_aSpellCDs[2]);
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell1 = GameEvent::CreateSVActionSpell(builder,
                                                     this->GetUID(),
                                                     2);
        auto event = GameEvent::CreateMessage(builder,
                                              GameEvent::Events_SVActionSpell,
                                              spell1.Union());
        builder.Finish(event);
        
        m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                            builder.GetBufferPointer() + builder.GetSize());
        
        WarriorArmorUp * pArmUp = new WarriorArmorUp(5s,
                                                     4);
        pArmUp->SetTargetUnit(this);
        this->ApplyEffect(pArmUp);
    }
}
