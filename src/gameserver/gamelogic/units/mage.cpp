//
//  mage.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "mage.hpp"

#include "../effect.hpp"
#include "../gameworld.hpp"
#include "../../GameMessage.h"

#include <chrono>
using namespace std::chrono_literals;


Mage::Mage(GameWorld& world, uint32_t uid)
: Hero(world, uid)
{
    _heroType = Hero::Type::MAGE;
    _moveSpeed = 0.3;
    _health = SimpleProperty<>(50, 0, 50);
    _damage = SimpleProperty<>(18, 0, 100);
    _armor = SimpleProperty<>(2, 0, 100);
    _resistance = SimpleProperty<>(6, 0, 100);
    
        // spell 1 cd
    _cdManager.AddSpell(30s);
    
        // spell 2 cd
    _cdManager.AddSpell(0s);
    
        // spell 3 cd
    _cdManager.AddSpell(10s);
}


void
Mage::SpellCast(const GameMessage::CLActionSpell* spell)
{
        // teleport cast (0 spell)
    if(spell->spell_id() == 0 &&
       _cdManager.SpellReady(0))
    {
            // set up CD
        _cdManager.Restart(0);
        
        Point<> new_pos;
        while(this->GetPosition().Distance(new_pos = _world.GetRandomPosition()) > 10.0)
        {
        }
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell_info = GameMessage::CreateMageTeleport(builder,
                                                          new_pos.x,
                                                          new_pos.y);
        auto spell = GameMessage::CreateSpell(builder,
                                              GameMessage::Spells_MageTeleport,
                                              spell_info.Union());
        auto spell1 = GameMessage::CreateSVActionSpell(builder,
                                                       this->GetUID(),
                                                       0,
                                                       spell);
        auto event = GameMessage::CreateMessage(builder,
                                                0,
                                                GameMessage::Messages_SVActionSpell,
                                                spell1.Union());
        builder.Finish(event);
        
        _world._outputMessages.push_back(std::make_shared<MessageBuffer>(builder.GetCurrentBufferPointer(),
                                                                         builder.GetBufferPointer() + builder.GetSize()));
        
        SetPosition(new_pos);
    }
        // attack cast (1 spell)
    else if(const auto enemy = _duelTarget.lock();
            enemy && spell->spell_id() == 1 && _cdManager.SpellReady(1))
    {
            // Log damage event
        _logger.Info() << "Attack " << enemy->GetName() << " for " << _damage;
        
            // set up CD
        _cdManager.Restart(1);
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell_info = GameMessage::CreateMageAttack(builder,
                                                        enemy->GetUID(),
                                                        _damage);
        auto spell = GameMessage::CreateSpell(builder,
                                              GameMessage::Spells_MageAttack,
                                              spell_info.Union());
        auto spell1 = GameMessage::CreateSVActionSpell(builder,
                                                       this->GetUID(),
                                                       1,
                                                       spell);
        auto event = GameMessage::CreateMessage(builder,
                                                0,
                                                GameMessage::Messages_SVActionSpell,
                                                spell1.Union());
        builder.Finish(event);

        _world._outputMessages.push_back(std::make_shared<MessageBuffer>(builder.GetCurrentBufferPointer(),
                                                                  builder.GetBufferPointer() + builder.GetSize()));
        
            // deal MAGIC damage
        DamageDescriptor dmgDescr;
        dmgDescr.DealerName = _name;
        dmgDescr.Value = _damage;
        dmgDescr.Type = Unit::DamageDescriptor::DamageType::MAGICAL;
        enemy->TakeDamage(dmgDescr);
    }
        // frostbolt casted (2 spell)
    else if(const auto enemy = _duelTarget.lock();
            enemy && spell->spell_id() == 2 && _cdManager.SpellReady(2))
    {
            // Log frostbolt casted
        _logger.Info() << "Casted frostbolt on " << enemy->GetName();

            // set up CD
        _cdManager.Restart(2);
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell1 = GameMessage::CreateMageFreeze(builder,
                                                    enemy->GetUID());
        auto spell = GameMessage::CreateSpell(builder,
                                              GameMessage::Spells_MageFreeze,
                                              spell1.Union());
        auto cl_spell = GameMessage::CreateSVActionSpell(builder,
                                                         this->GetUID(),
                                                         2,
                                                         spell);
        auto event = GameMessage::CreateMessage(builder,
                                                0,
                                                GameMessage::Messages_SVActionSpell,
                                                cl_spell.Union());
        builder.Finish(event);
        
        _world._outputMessages.push_back(std::make_shared<MessageBuffer>(builder.GetCurrentBufferPointer(),
                                                                         builder.GetBufferPointer() + builder.GetSize()));
        
            // apply freeze effect
        auto mageFreeze = std::make_shared<MageFreeze>(3s);
        mageFreeze->SetTargetUnit(enemy);
        enemy->ApplyEffect(mageFreeze);
    }
}
