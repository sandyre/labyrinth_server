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
        this->SetLogicalPosition(new_pos);
        
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
    }
        // frostbolt casted (1 spell)
    else if(spell->spell_id() == 1 &&
            std::get<0>(m_aSpellCDs[1]) == true)
    {
            // apply freeze effect
        MageFreeze * pFreeze = new MageFreeze(3s);
        pFreeze->SetTargetUnit(m_pDuelTarget);
        m_pDuelTarget->ApplyEffect(pFreeze);
        
            // set up CD
        std::get<0>(m_aSpellCDs[1]) = false;
        std::get<1>(m_aSpellCDs[1]) = std::get<2>(m_aSpellCDs[1]);
        
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell1 = GameEvent::CreateMageFreeze(builder,
                                                  m_pDuelTarget->GetUID());
        auto spell = GameEvent::CreateSpell(builder,
                                            GameEvent::Spells_MageFreeze,
                                            spell1.Union());
        auto cl_spell = GameEvent::CreateSVActionSpell(builder,
                                                       this->GetUID(),
                                                       1,
                                                       spell);
        auto event = GameEvent::CreateMessage(builder,
                                              GameEvent::Events_SVActionSpell,
                                              cl_spell.Union());
        builder.Finish(event);
        
        m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                            builder.GetBufferPointer() + builder.GetSize());
    }
}

void
Mage::update(std::chrono::milliseconds delta)
{
    Hero::update(delta);
}
