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
Rogue::update(std::chrono::microseconds delta)
{
    Hero::update(delta);
}
