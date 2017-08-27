//
//  rogue.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "rogue.hpp"

#include "../effect.hpp"
#include "../gameworld.hpp"
#include "../../GameMessage.h"

#include <chrono>
using namespace std::chrono_literals;

using Attributes = GameObject::Attributes;


Rogue::Rogue(GameWorld& world, uint32_t uid)
: Hero(world, uid)
{
    _heroType = Hero::Type::ROGUE;
    _moveSpeed = 0.1;
//    _maxHealth = _health = 75;
//    _baseDamage = _actualDamage = 8;
    _armor = 2;
    
        // spell 1 cd
    _cdManager.AddSpell(30s);
    
        // spell 2 cd
    _cdManager.AddSpell(15s);
}


void
Rogue::SpellCast(const GameMessage::CLActionSpell* spell)
{
        // invisibility cast (0 spell)
    if(spell->spell_id() == 0 &&
       _cdManager.SpellReady(0))
    {
            // set up CD
        _cdManager.Restart(0);

        auto invis = std::make_shared<RogueInvisibility>(5s);
        invis->SetTargetUnit(std::static_pointer_cast<Unit>(shared_from_this()));
        this->ApplyEffect(invis);
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell1 = GameMessage::CreateSVActionSpell(builder,
                                                     this->GetUID(),
                                                     0);
        auto event = GameMessage::CreateMessage(builder,
                                              0,
                                              GameMessage::Messages_SVActionSpell,
                                              spell1.Union());
        builder.Finish(event);
        
//        _world._outputEvents.emplace(builder.GetBufferPointer(),
//                                     builder.GetBufferPointer() + builder.GetSize());
    }
        // missing knife cast (1 spell)
    else if(spell->spell_id() == 1)
    {
        
    }
}


void
Rogue::TakeItem(std::shared_ptr<Item> item)
{
    Unit::TakeItem(item);
}


void
Rogue::update(std::chrono::microseconds delta)
{
    Hero::update(delta);
}
