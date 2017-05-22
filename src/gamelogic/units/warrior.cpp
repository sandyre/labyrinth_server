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
    m_aSpellCDs.push_back(std::make_tuple(true, 0s, 5s));
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
        
        WarriorDash * pDash = new WarriorDash(3s,
                                              5.5);
        pDash->SetTargetUnit(this);
        this->ApplyEffect(pDash);
        
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
        // warrior armor up cast (1 spell)
    else if(spell->spell_id() == 1 &&
            std::get<0>(m_aSpellCDs[1]) == true) // check CD
    {
            // set up CD
        std::get<0>(m_aSpellCDs[1]) = false;
        std::get<1>(m_aSpellCDs[1]) = std::get<2>(m_aSpellCDs[1]);
        
        WarriorArmorUp * pArmUp = new WarriorArmorUp(5s,
                                                     4);
        pArmUp->SetTargetUnit(this);
        this->ApplyEffect(pArmUp);
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell1 = GameEvent::CreateSVActionSpell(builder,
                                                     this->GetUID(),
                                                     1);
        auto event = GameEvent::CreateMessage(builder,
                                              GameEvent::Events_SVActionSpell,
                                              spell1.Union());
        builder.Finish(event);
        
        m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                            builder.GetBufferPointer() + builder.GetSize());
    }
}
