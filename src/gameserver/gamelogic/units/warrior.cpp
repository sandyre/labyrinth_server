//
//  warrior.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "warrior.hpp"

#include "../gameworld.hpp"
#include "../../GameMessage.h"
#include "../effect.hpp"

#include <chrono>
using namespace std::chrono_literals;

Warrior::Warrior(GameWorld& world)
: Hero(world)
{
    _heroType = Hero::Type::WARRIOR;
    _moveSpeed = 0.4;
    _maxHealth = _health = 80;
    _baseDamage = _actualDamage = 10;
    _armor = 6;
    _magResistance = 2;
    
        // spell 1 cd
    _spellsCDs.push_back(std::make_tuple(true, 0s, 10s));
    
        // spell 2 cd
    _spellsCDs.push_back(std::make_tuple(true, 0s, 0s));
    
        // spell 3 cd
    _spellsCDs.push_back(std::make_tuple(true, 0s, 10s));
}

void
Warrior::SpellCast(const GameMessage::CLActionSpell* spell)
{
        // warrior dash cast (0 spell)
    if(spell->spell_id() == 0 &&
       std::get<0>(_spellsCDs[0]) == true) // check CD
    {
            // set up CD
        std::get<0>(_spellsCDs[0]) = false;
        std::get<1>(_spellsCDs[0]) = std::get<2>(_spellsCDs[0]);
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell1 = GameMessage::CreateSVActionSpell(builder,
                                                     this->GetUID(),
                                                     0);
        auto event = GameMessage::CreateMessage(builder,
                                              0,
                                              GameMessage::Messages_SVActionSpell,
                                              spell1.Union());
        builder.Finish(event);
        
        _world._outputEvents.emplace(builder.GetBufferPointer(),
                                     builder.GetBufferPointer() + builder.GetSize());

        auto warDash = std::make_shared<WarriorDash>(3s, 5.5);
        warDash->SetTargetUnit(std::static_pointer_cast<Unit>(shared_from_this()));
        this->ApplyEffect(warDash);
    }
    else if(spell->spell_id() == 1 &&
            std::get<0>(_spellsCDs[1]) == true) // warrior attack (2 spell)
    {
        if(_duelTarget == nullptr)
            return;
        
            // Log damage event
        _world._logger.Info() << this->GetName() << " " << _actualDamage << " PHYS DMG TO " << _duelTarget->GetName();
        
            // set up CD
        std::get<0>(_spellsCDs[1]) = false;
        std::get<1>(_spellsCDs[1]) = std::get<2>(_spellsCDs[1]);
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell_info = GameMessage::CreateWarriorAttack(builder,
                                                      _duelTarget->GetUID(),
                                                      _actualDamage);
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
        
        _world._outputEvents.emplace(builder.GetBufferPointer(),
                                     builder.GetBufferPointer() + builder.GetSize());
        
            // deal PHYSICAL damage
        DamageDescriptor dmgDescr;
        dmgDescr.DealerName = _name;
        dmgDescr.Value = _actualDamage;
        dmgDescr.Type = DamageDescriptor::DamageType::PHYSICAL;
        _duelTarget->TakeDamage(dmgDescr);
    }
        // warrior armor up cast (2 spell)
    else if(spell->spell_id() == 2 &&
            std::get<0>(_spellsCDs[2]) == true) // check CD
    {
            // set up CD
        std::get<0>(_spellsCDs[2]) = false;
        std::get<1>(_spellsCDs[2]) = std::get<2>(_spellsCDs[2]);
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell1 = GameMessage::CreateSVActionSpell(builder,
                                                     this->GetUID(),
                                                     2);
        auto event = GameMessage::CreateMessage(builder,
                                              0,
                                              GameMessage::Messages_SVActionSpell,
                                              spell1.Union());
        builder.Finish(event);
        
        _world._outputEvents.emplace(builder.GetBufferPointer(),
                                     builder.GetBufferPointer() + builder.GetSize());

        auto armorUp = std::make_shared<WarriorArmorUp>(5s, 4);
        armorUp->SetTargetUnit(std::static_pointer_cast<Unit>(shared_from_this()));
        this->ApplyEffect(armorUp);
    }
}