//
//  warrior.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "warrior.hpp"

#include "../gameworld.hpp"
#include "../../gsnet_generated.h"
#include "../effect.hpp"

#include <chrono>
using namespace std::chrono_literals;

Warrior::Warrior()
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
Warrior::SpellCast(const GameEvent::CLActionSpell* spell)
{
        // warrior dash cast (0 spell)
    if(spell->spell_id() == 0 &&
       std::get<0>(_spellsCDs[0]) == true) // check CD
    {
            // set up CD
        std::get<0>(_spellsCDs[0]) = false;
        std::get<1>(_spellsCDs[0]) = std::get<2>(_spellsCDs[0]);
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell1 = GameEvent::CreateSVActionSpell(builder,
                                                     this->GetUID(),
                                                     0);
        auto event = GameEvent::CreateMessage(builder,
                                              0,
                                              GameEvent::Events_SVActionSpell,
                                              spell1.Union());
        builder.Finish(event);
        
        _gameWorld->_outputEvents.emplace(builder.GetBufferPointer(),
                                            builder.GetBufferPointer() + builder.GetSize());
        
        WarriorDash * pDash = new WarriorDash(3s,
                                              5.5);
        pDash->SetTargetUnit(this);
        this->ApplyEffect(pDash);
    }
    else if(spell->spell_id() == 1 &&
            std::get<0>(_spellsCDs[1]) == true) // warrior attack (2 spell)
    {
        if(_duelTarget == nullptr)
            return;
        
            // Log damage event
        auto& logger = _gameWorld->_logger;
        logger.Info() << this->GetName() << " " << _actualDamage << " PHYS DMG TO " << _duelTarget->GetName() << End();
        
            // set up CD
        std::get<0>(_spellsCDs[1]) = false;
        std::get<1>(_spellsCDs[1]) = std::get<2>(_spellsCDs[1]);
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell_info = GameEvent::CreateWarriorAttack(builder,
                                                      _duelTarget->GetUID(),
                                                      _actualDamage);
        auto spell = GameEvent::CreateSpell(builder,
                                            GameEvent::Spells_WarriorAttack,
                                            spell_info.Union());
        auto spell1 = GameEvent::CreateSVActionSpell(builder,
                                                     this->GetUID(),
                                                     1,
                                                     spell);
        auto event = GameEvent::CreateMessage(builder,
                                              0,
                                              GameEvent::Events_SVActionSpell,
                                              spell1.Union());
        builder.Finish(event);
        
        _gameWorld->_outputEvents.emplace(builder.GetBufferPointer(),
                                            builder.GetBufferPointer() + builder.GetSize());
        
            // deal PHYSICAL damage
        _duelTarget->TakeDamage(_actualDamage,
                                  Unit::DamageType::PHYSICAL,
                                  this);
    }
        // warrior armor up cast (2 spell)
    else if(spell->spell_id() == 2 &&
            std::get<0>(_spellsCDs[2]) == true) // check CD
    {
            // set up CD
        std::get<0>(_spellsCDs[2]) = false;
        std::get<1>(_spellsCDs[2]) = std::get<2>(_spellsCDs[2]);
        
        flatbuffers::FlatBufferBuilder builder;
        auto spell1 = GameEvent::CreateSVActionSpell(builder,
                                                     this->GetUID(),
                                                     2);
        auto event = GameEvent::CreateMessage(builder,
                                              0,
                                              GameEvent::Events_SVActionSpell,
                                              spell1.Union());
        builder.Finish(event);
        
        _gameWorld->_outputEvents.emplace(builder.GetBufferPointer(),
                                            builder.GetBufferPointer() + builder.GetSize());
        
        WarriorArmorUp * pArmUp = new WarriorArmorUp(5s,
                                                     4);
        pArmUp->SetTargetUnit(this);
        this->ApplyEffect(pArmUp);
    }
}
