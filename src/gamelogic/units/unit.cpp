//
//  unit.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "unit.hpp"

#include "../gameworld.hpp"
#include "../../gsnet_generated.h"

#include <chrono>
using namespace std::chrono_literals;

using Attributes = GameObject::Attributes;

Unit::Unit() :
m_eUnitType(Unit::Type::UNDEFINED),
m_eState(Unit::State::UNDEFINED),
m_eOrientation(Unit::Orientation::DOWN),
m_sName("Unit"),
m_nBaseDamage(0),
m_nActualDamage(0),
m_nHealth(0),
m_nMHealth(0),
m_nMoveSpeed(0.5),
m_pDuelTarget(nullptr)
{
    m_eObjType = GameObject::Type::UNIT;
    m_nAttributes |= GameObject::Attributes::DAMAGABLE;
    m_nAttributes |= GameObject::Attributes::MOVABLE;
    m_nAttributes |= GameObject::Attributes::DUELABLE;
}

Unit::Type
Unit::GetUnitType() const
{
    return m_eUnitType;
}

Unit::State
Unit::GetState() const
{
    return m_eState;
}

Unit::Orientation
Unit::GetOrientation() const
{
    return m_eOrientation;
}

std::string
Unit::GetName() const
{
    return m_sName;
}

void
Unit::SetName(const std::string& name)
{
    m_sName = name;
}

int16_t
Unit::GetDamage() const
{
    return m_nActualDamage;
}

int16_t
Unit::GetHealth() const
{
    return m_nHealth;
}

int16_t
Unit::GetMaxHealth() const
{
    return m_nMHealth;
}

Unit * const
Unit::GetDuelTarget() const
{
    return m_pDuelTarget;
}

std::vector<Item*>&
Unit::GetInventory()
{
    return m_aInventory;
}

/*
 *
 * Animations
 *
 */

void
Unit::UpdateStats()
{
    int new_dmg = m_nBaseDamage;
    for(auto item : m_aInventory)
    {
        if(item->GetType() == Item::Type::SWORD)
            new_dmg += 6;
    }
    m_nActualDamage = new_dmg;
}

void
Unit::TakeItem(Item * item)
{
    item->SetCarrierID(this->GetUID());
    m_aInventory.push_back(item);
    
    flatbuffers::FlatBufferBuilder builder;
    auto take = GameEvent::CreateSVActionItem(builder,
                                              this->GetUID(),
                                              item->GetUID(),
                                              GameEvent::ActionItemType_TAKE);
    auto msg = GameEvent::CreateMessage(builder,
                                        GameEvent::Events_SVActionItem,
                                        take.Union());
    builder.Finish(msg);
    m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
}

void
Unit::Spawn(Point2 log_pos)
{
        // Log spawn event
    auto m_pLogSystem = m_poGameWorld->m_pLogSystem;
    auto& m_oLogBuilder = m_poGameWorld->m_oLogBuilder;
    m_oLogBuilder << this->GetName() << " SPWN AT (" << log_pos.x << ";" << log_pos.y << ")";
    m_pLogSystem->Write(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
    m_eState = Unit::State::WALKING;
    m_nAttributes = GameObject::Attributes::MOVABLE |
                    GameObject::Attributes::VISIBLE |
                    GameObject::Attributes::DUELABLE |
                    GameObject::Attributes::DAMAGABLE;
    m_nHealth = m_nMHealth;
    
    m_stLogPosition = log_pos;
    
    flatbuffers::FlatBufferBuilder builder;
    auto spawn = GameEvent::CreateSVSpawnPlayer(builder,
                                                this->GetUID(),
                                                log_pos.x,
                                                log_pos.y);
    auto msg = GameEvent::CreateMessage(builder,
                                        GameEvent::Events_SVSpawnPlayer,
                                        spawn.Union());
    builder.Finish(msg);
    m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
}

void
Unit::Respawn(Point2 log_pos)
{
        // Log respawn event
    auto m_pLogSystem = m_poGameWorld->m_pLogSystem;
    auto& m_oLogBuilder = m_poGameWorld->m_oLogBuilder;
    m_oLogBuilder << this->GetName() << " RESP AT (" << log_pos.x << ";" << log_pos.y << ")";
    m_pLogSystem->Write(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
    m_eState = Unit::State::WALKING;
    m_nAttributes = GameObject::Attributes::MOVABLE |
    GameObject::Attributes::VISIBLE |
    GameObject::Attributes::DUELABLE |
    GameObject::Attributes::DAMAGABLE;
    m_nHealth = m_nMHealth;
    
    m_stLogPosition = log_pos;
    
    flatbuffers::FlatBufferBuilder builder;
    auto resp = GameEvent::CreateSVRespawnPlayer(builder,
                                                 this->GetUID(),
                                                 log_pos.x,
                                                 log_pos.y);
    auto msg = GameEvent::CreateMessage(builder,
                                        GameEvent::Events_SVRespawnPlayer,
                                        resp.Union());
    builder.Finish(msg);
    m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
}

void
Unit::Die()
{
        // Log dead event
    auto m_pLogSystem = m_poGameWorld->m_pLogSystem;
    auto& m_oLogBuilder = m_poGameWorld->m_oLogBuilder;
    m_oLogBuilder << this->GetName() << " DIED AT (" << m_stLogPosition.x << ";" << m_stLogPosition.y << ")";
    m_pLogSystem->Write(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
    if(m_pDuelTarget != nullptr)
        EndDuel();
    
    m_eState = Unit::State::DEAD;
    m_nAttributes = GameObject::Attributes::PASSABLE;
    m_nHealth = 0;
    m_poGameWorld->m_aRespawnQueue.push_back(std::make_pair(3s, this));
}

void
Unit::Move(Point2 log_pos)
{
        // firstly - check if it can go there (no unpassable objects)
    for(auto object : m_poGameWorld->m_apoObjects)
    {
        if(object->GetLogicalPosition() == log_pos &&
           !(object->GetAttributes() & GameObject::Attributes::PASSABLE))
        {
            return; // there is an unpassable object
        }
    }
        // Log move event
    auto m_pLogSystem = m_poGameWorld->m_pLogSystem;
    auto& m_oLogBuilder = m_poGameWorld->m_oLogBuilder;
    m_oLogBuilder << this->GetName() << " MOVE (" << m_stLogPosition.x
    << ";" << m_stLogPosition.y << ") -> (" << log_pos.x << ";" << log_pos.y << ")";
    m_pLogSystem->Write(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
    this->SetLogicalPosition(log_pos);
    
    flatbuffers::FlatBufferBuilder builder;
    auto move = GameEvent::CreateSVActionMove(builder,
                                              this->GetUID(),
                                              log_pos.x,
                                              log_pos.y);
    auto msg = GameEvent::CreateMessage(builder,
                                        GameEvent::Events_SVActionMove,
                                        move.Union());
    builder.Finish(msg);
    m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
}

void
Unit::Attack()
{
    auto m_pLogSystem = m_poGameWorld->m_pLogSystem;
    auto& m_oLogBuilder = m_poGameWorld->m_oLogBuilder;
        // Log duel start event
    m_oLogBuilder << this->GetName() << " ATK TO " << m_pDuelTarget->GetName()
    << " for " << m_nActualDamage;
    m_pLogSystem->Write(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
    m_pDuelTarget->TakeDamage(m_nActualDamage);
    
    flatbuffers::FlatBufferBuilder builder;
    auto atk = GameEvent::CreateSVActionDuel(builder,
                                             this->GetUID(),
                                             m_pDuelTarget->GetUID(),
                                             GameEvent::ActionDuelType_ATTACK);
    auto msg = GameEvent::CreateMessage(builder,
                                        GameEvent::Events_SVActionDuel,
                                        atk.Union());
    builder.Finish(msg);
    m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
    builder.Clear();
    
    if(m_pDuelTarget->GetState() == Unit::State::DEAD)
    {
            // Log duel end (kill)
        m_oLogBuilder << this->GetName() << " DUEL KILL " << m_pDuelTarget->GetName();
        m_pLogSystem->Write(m_oLogBuilder.str());
        m_oLogBuilder.str("");
        auto kill = GameEvent::CreateSVActionDuel(builder,
                                                  this->GetUID(),
                                                  m_pDuelTarget->GetUID(),
                                                  GameEvent::ActionDuelType_KILL);
        auto msg = GameEvent::CreateMessage(builder,
                                            GameEvent::Events_SVActionDuel,
                                            kill.Union());
        builder.Finish(msg);
        m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                            builder.GetBufferPointer() + builder.GetSize());
        builder.Clear();
        EndDuel();
    }
}

void
Unit::TakeDamage(int16_t dmg)
{
        // log damage take
    auto m_pLogSystem = m_poGameWorld->m_pLogSystem;
    auto& m_oLogBuilder = m_poGameWorld->m_oLogBuilder;
    m_oLogBuilder << this->GetName() << " ATK FROM " << m_pDuelTarget->GetName()
    << " HP: " << m_nHealth << " -> " << m_nHealth - dmg;
    m_pLogSystem->Write(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
    if(m_nHealth - dmg <= 0)
    {
        Die();
    }
    else
    {
        m_nHealth -= dmg;
    }
}

void
Unit::StartDuel(Unit * enemy)
{
        // FIXME: each duel sends 2 msges about duel start.
        // client now can eat it, but server should also be reworked
    
        // Log duel start event
    auto m_pLogSystem = m_poGameWorld->m_pLogSystem;
    auto& m_oLogBuilder = m_poGameWorld->m_oLogBuilder;
    m_oLogBuilder << "DUEL START  " <<
    this->GetName() << " and " << enemy->GetName();
    m_pLogSystem->Write(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
    m_eState = Unit::State::DUEL;
    m_nAttributes ^= Attributes::DUELABLE;
    
    m_pDuelTarget = enemy;
    
    flatbuffers::FlatBufferBuilder builder;
    auto duel = GameEvent::CreateSVActionDuel(builder,
                                              this->GetUID(),
                                              enemy->GetUID(),
                                              GameEvent::ActionDuelType_STARTED);
    auto msg = GameEvent::CreateMessage(builder,
                                        GameEvent::Events_SVActionDuel,
                                        duel.Union());
    builder.Finish(msg);
    
    m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
}

void
Unit::EndDuel()
{
        // Log duel-end event
    auto m_pLogSystem = m_poGameWorld->m_pLogSystem;
    auto& m_oLogBuilder = m_poGameWorld->m_oLogBuilder;
    m_oLogBuilder << this->GetName() << " DUEL END W " << m_pDuelTarget->GetName();
    m_pLogSystem->Write(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
    m_eState = Unit::State::WALKING;
    m_nAttributes ^= Attributes::DUELABLE;
    
    m_pDuelTarget = nullptr;
}
