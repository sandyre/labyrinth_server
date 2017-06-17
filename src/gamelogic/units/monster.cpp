//
//  monster.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "monster.hpp"

#include "../../gsnet_generated.h"
#include "../gameworld.hpp"

#include <chrono>
using namespace std::chrono_literals;

Monster::Monster() :
m_pChasingUnit(nullptr)
{
    _unitType = Unit::Type::MONSTER;
    _name = "Skeleton";
    
    _baseDamage = _actualDamage = 10;
    _maxHealth = _health = 50;
    _armor = 2;
    _magResistance = 2;
    
        // spell 1 cd
    _spellsCDs.push_back(std::make_tuple(false, 3s, 3s));
    
        // spell 1 seq
    InputSequence atk_seq(5);
    m_aCastSequences.push_back(atk_seq);
    
    m_msCastTime = 600ms;
    m_msACastTime = 0ms;
    
    m_msMoveCD = 2s;
    m_msMoveACD = 0s;
}

void
Monster::update(std::chrono::microseconds delta)
{
    Unit::update(delta);
    
    m_msACastTime -= delta;
    if(m_msACastTime < 0ms)
        m_msACastTime = 0ms;
    
//        // find target to chase
//    if(m_pChasingUnit == nullptr)
//    {
//        for(auto obj : m_poGameWorld->m_apoObjects)
//        {
//            if(obj->GetObjType() == GameObject::Type::UNIT &&
//               obj->GetUID() != this->GetUID() &&
//               Distance(obj->GetLogicalPosition(), this->GetLogicalPosition()) <= 4.0 &&
//               dynamic_cast<Unit*>(obj)->GetUnitAttributes() & Unit::Attributes::DUELABLE)
//            {
//                m_pChasingUnit = dynamic_cast<Unit*>(obj);
//                break;
//            }
//        }
//    }
    if(!(_unitAttributes & Unit::Attributes::INPUT))
        return;
    
    switch (_state)
    {
        case Unit::State::DUEL:
        {
            if(m_msACastTime == 0ms)
            {
                m_aCastSequences[0].sequence.pop_back();
                m_msACastTime = m_msCastTime;
                
                if(m_aCastSequences[0].sequence.empty())
                {
                    if(_duelTarget == nullptr)
                        return;
                    
                        // Log damage event
                    auto& m_pLogSystem = _gameWorld->_logSystem;
                    auto& m_oLogBuilder = _gameWorld->_logBuilder;
                    m_oLogBuilder << this->GetName() << " " << _actualDamage << " PHYS DMG TO " << _duelTarget->GetName();
                    m_pLogSystem.Info(m_oLogBuilder.str());
                    m_oLogBuilder.str("");
                    
                        // set up CD
                    std::get<0>(_spellsCDs[0]) = false;
                    std::get<1>(_spellsCDs[0]) = std::get<2>(_spellsCDs[0]);
                    
                    flatbuffers::FlatBufferBuilder builder;
                    auto spell_info = GameEvent::CreateMonsterAttack(builder,
                                                                     _duelTarget->GetUID(),
                                                                     _actualDamage);
                    auto spell = GameEvent::CreateSpell(builder,
                                                        GameEvent::Spells_MonsterAttack,
                                                        spell_info.Union());
                    auto spell1 = GameEvent::CreateSVActionSpell(builder,
                                                                 this->GetUID(),
                                                                 0,
                                                                 spell);
                    auto event = GameEvent::CreateMessage(builder,
                                                          GameEvent::Events_SVActionSpell,
                                                          spell1.Union());
                    builder.Finish(event);
                    
                    _gameWorld->_outputEvents.emplace(builder.GetBufferPointer(),
                                                        builder.GetBufferPointer() + builder.GetSize());
                    
                        // deal PHYSICAL damage
                    _duelTarget->TakeDamage(_actualDamage,
                                              Unit::DamageType::PHYSICAL,
                                              this);
                    
                    m_aCastSequences[0].Refresh();
                }
            }
            
            break;
        }
            
        default:
            break;
    }
}

void
Monster::Spawn(Point2 log_pos)
{
    _state = Unit::State::WALKING;
    _objAttributes = GameObject::Attributes::MOVABLE |
    GameObject::Attributes::VISIBLE |
    GameObject::Attributes::DAMAGABLE;
    _unitAttributes = Unit::Attributes::INPUT |
    Unit::Attributes::ATTACK |
    Unit::Attributes::DUELABLE;
    _health = _maxHealth;
    
    _logPos = log_pos;
    
    flatbuffers::FlatBufferBuilder builder;
    auto spawn = GameEvent::CreateSVSpawnMonster(builder,
                                                 this->GetUID(),
                                                 log_pos.x,
                                                 log_pos.y);
    auto msg = GameEvent::CreateMessage(builder,
                                        GameEvent::Events_SVSpawnMonster,
                                        spawn.Union());
    builder.Finish(msg);
    _gameWorld->_outputEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
}

void
Monster::Die(Unit * killer)
{
    if(killer->GetDuelTarget() == this)
    {
        killer->EndDuel();
    }
        // Log dead event
    auto& m_pLogSystem = _gameWorld->_logSystem;
    auto& m_oLogBuilder = _gameWorld->_logBuilder;
    m_oLogBuilder << this->GetName() << " KILLED BY " << killer->GetName() << " DIED AT (" << _logPos.x << ";" << _logPos.y << ")";
    m_pLogSystem.Info(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
        // drop items
    while(!_inventory.empty())
    {
        this->DropItem(0);
    }
    
    if(_duelTarget != nullptr)
        EndDuel();
    
    flatbuffers::FlatBufferBuilder builder;
    auto move = GameEvent::CreateSVActionDeath(builder,
                                               this->GetUID(),
                                               killer->GetUID());
    auto msg = GameEvent::CreateMessage(builder,
                                        GameEvent::Events_SVActionDeath,
                                        move.Union());
    builder.Finish(msg);
    _gameWorld->_outputEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
    
    _state = Unit::State::DEAD;
    _objAttributes = GameObject::Attributes::PASSABLE;
    _unitAttributes = 0;
    _health = 0;
}
