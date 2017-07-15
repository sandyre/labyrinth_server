//
//  mage.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "mage.hpp"

#include "../gameworld.hpp"
#include "../../GameMessage.h"

#include "../effect.hpp"

#include <chrono>
using namespace std::chrono_literals;

Mage::Mage(GameWorld& world)
: Hero(world)
{
    _heroType = Hero::Type::MAGE;
    _moveSpeed = 0.3;
    _maxHealth = _health = 50;
    _baseDamage = _actualDamage = 18;
    _armor = 2;
    _magResistance = 6;
    
        // spell 1 cd
    _spellsCDs.push_back(std::make_tuple(true, 0s, 30s));
    
        // spell 2 cd
    _spellsCDs.push_back(std::make_tuple(true, 0s, 0s));
    
        // spell 3 cd
    _spellsCDs.push_back(std::make_tuple(true, 0s, 10s));
}

void
Mage::SpellCast(const GameMessage::CLActionSpell* spell)
{
        // teleport cast (0 spell)
    if(spell->spell_id() == 0 &&
       std::get<0>(_spellsCDs[0]) == true)
    {
            // set up CD
        std::get<0>(_spellsCDs[0]) = false;
        std::get<1>(_spellsCDs[0]) = std::get<2>(_spellsCDs[0]);
        
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
        
        _world._outputEvents.emplace(builder.GetBufferPointer(),
                                            builder.GetBufferPointer() + builder.GetSize());
        
        SetPosition(new_pos);
    }
        // attack cast (1 spell)
    else if(spell->spell_id() == 1 &&
            std::get<0>(_spellsCDs[1]) == true)
    {
        if(_duelTarget == nullptr)
            return;
        
            // Log damage event
        _world._logger.Info() << this->GetName() << " " << _actualDamage << " MAG DMG TO " << _duelTarget->GetName();
        
            // set up CD
        std::get<0>(_spellsCDs[1]) = false;
        std::get<1>(_spellsCDs[1]) = std::get<2>(_spellsCDs[1]);
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell_info = GameMessage::CreateMageAttack(builder,
                                                      _duelTarget->GetUID(),
                                                      _actualDamage);
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

        _world._outputEvents.emplace(builder.GetBufferPointer(),
                                     builder.GetBufferPointer() + builder.GetSize());
        
            // deal MAGIC damage
        DamageDescriptor dmgDescr;
        dmgDescr.DealerName = _name;
        dmgDescr.Value = _actualDamage;
        dmgDescr.Type = Unit::DamageDescriptor::DamageType::MAGICAL;
        _duelTarget->TakeDamage(dmgDescr);
    }
        // frostbolt casted (2 spell)
    else if(spell->spell_id() == 2 &&
            std::get<0>(_spellsCDs[2]) == true)
    {
        if(_duelTarget == nullptr)
            return;
        
            // set up CD
        std::get<0>(_spellsCDs[2]) = false;
        std::get<1>(_spellsCDs[2]) = std::get<2>(_spellsCDs[2]);
        
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell1 = GameMessage::CreateMageFreeze(builder,
                                                  _duelTarget->GetUID());
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
        
        _world._outputEvents.emplace(builder.GetBufferPointer(),
                                     builder.GetBufferPointer() + builder.GetSize());
        
            // apply freeze effect
        auto mageFreeze = std::make_shared<MageFreeze>(3s);
        mageFreeze->SetTargetUnit(_duelTarget);
        _duelTarget->ApplyEffect(mageFreeze);
    }
}

void
Mage::update(std::chrono::microseconds delta)
{
    Hero::update(delta);
}
