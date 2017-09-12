//
//  warrior.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "warrior.hpp"

#include "../effect.hpp"
#include "../gameworld.hpp"
#include "../../GameMessage.h"

#include <chrono>
using namespace std::chrono_literals;


Warrior::Warrior(GameWorld& world, uint32_t uid)
: Hero(world, uid)
{
    _heroType = Hero::Type::WARRIOR;
    _moveSpeed = 0.4;
    _health = SimpleProperty<>(80, 0, 80);
    _damage = SimpleProperty<>(10, 0, 100);
    _armor = SimpleProperty<>(6, 0, 100);
    _resistance = SimpleProperty<>(2, 0, 100);
    
        // spell 1 cd
    _cdManager.AddSpell(10s);
    
        // spell 2 cd
    _cdManager.AddSpell(0s);
    
        // spell 3 cd
    _cdManager.AddSpell(10s);
}


void
Warrior::SpellCast(const GameMessage::CLActionSpell* spell)
{
        // warrior dash cast (0 spell)
    if(spell->spell_id() == 0 &&
       _cdManager.SpellReady(0)) // check CD
    {
            // set up CD
        _cdManager.Restart(0);
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell1 = GameMessage::CreateSVActionSpell(builder,
                                                       this->GetUID(),
                                                       0);
        auto event = GameMessage::CreateMessage(builder,
                                                0,
                                                GameMessage::Messages_SVActionSpell,
                                                spell1.Union());
        builder.Finish(event);
        
        _world._outputMessages.push_back(std::make_shared<MessageBuffer>(builder.GetCurrentBufferPointer(),
                                                                         builder.GetBufferPointer() + builder.GetSize()));

        auto warDash = std::make_shared<WarriorDash>(3s, 5.5);
        warDash->SetTargetUnit(std::static_pointer_cast<Unit>(shared_from_this()));
        this->ApplyEffect(warDash);
    }
    else if(const auto enemy = _duelTarget.lock();
            enemy && spell->spell_id() == 1 && _cdManager.SpellReady(1)) // warrior attack (2 spell)
    {
            // Log damage event
        _logger.Info() << "Attack " << enemy->GetName() << " for " << _damage;
        
            // set up CD
        _cdManager.Restart(1);
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell_info = GameMessage::CreateWarriorAttack(builder,
                                                           enemy->GetUID(),
                                                           _damage);
        auto spell = GameMessage::CreateSpell(builder,
                                              GameMessage::Spells_WarriorAttack,
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
        
            // deal PHYSICAL damage
        DamageDescriptor dmgDescr;
        dmgDescr.DealerName = _name;
        dmgDescr.Value = _damage;
        dmgDescr.Type = DamageDescriptor::DamageType::PHYSICAL;
        enemy->TakeDamage(dmgDescr);
    }
        // warrior armor up cast (2 spell)
    else if(const auto enemy = _duelTarget.lock();
            enemy && spell->spell_id() == 2 && _cdManager.SpellReady(2)) // check CD
    {
            // set up CD
        _cdManager.Restart(2);
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell1 = GameMessage::CreateSVActionSpell(builder,
                                                       this->GetUID(),
                                                       2);
        auto event = GameMessage::CreateMessage(builder,
                                                0,
                                                GameMessage::Messages_SVActionSpell,
                                                spell1.Union());
        builder.Finish(event);
        
        _world._outputMessages.push_back(std::make_shared<MessageBuffer>(builder.GetCurrentBufferPointer(),
                                                                         builder.GetBufferPointer() + builder.GetSize()));

        auto armorUp = std::make_shared<WarriorArmorUp>(5s, 4);
        armorUp->SetTargetUnit(std::static_pointer_cast<Unit>(shared_from_this()));
        this->ApplyEffect(armorUp);
    }
}
