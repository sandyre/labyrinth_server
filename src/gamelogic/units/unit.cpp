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
#include "effect.hpp"

#include <chrono>
using namespace std::chrono_literals;

using Attributes = GameObject::Attributes;

Unit::Unit() :
m_eUnitType(Unit::Type::UNDEFINED),
m_eState(Unit::State::UNDEFINED),
m_eOrientation(Unit::Orientation::DOWN),
m_sName("Unit"),
m_nBaseDamage(10),
m_nActualDamage(10),
m_nHealth(50),
m_nMHealth(50),
m_nArmor(2),
m_nMResistance(2),
m_nMoveSpeed(0.5),
m_pDuelTarget(nullptr)
{
    m_eObjType = GameObject::Type::UNIT;
    m_nObjAttributes |= GameObject::Attributes::DAMAGABLE;
    m_nObjAttributes |= GameObject::Attributes::MOVABLE;
    m_nUnitAttributes = Unit::Attributes::INPUT |
    Unit::Attributes::ATTACK |
    Unit::Attributes::DUELABLE;
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

int16_t
Unit::GetArmor() const
{
    return m_nArmor;
}

Unit * const
Unit::GetDuelTarget() const
{
    return m_pDuelTarget;
}

uint32_t
Unit::GetUnitAttributes() const
{
    return m_nUnitAttributes;
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
Unit::ApplyEffect(Effect * effect)
{
    effect->start();
    m_aAppliedEffects.push_back(effect);
}

void
Unit::update(std::chrono::microseconds delta)
{
    UpdateCDs(delta);
    for(auto effect : m_aAppliedEffects)
    {
        effect->update(delta);
        if(effect->GetState() == Effect::State::OVER)
        {
            effect->stop();
        }
    }
    m_aAppliedEffects.erase(std::remove_if(m_aAppliedEffects.begin(),
                                           m_aAppliedEffects.end(),
                                           [this](Effect * eff)
                                           {
                                               if(eff->GetState() == Effect::State::OVER)
                                               {
                                                   delete eff;
                                                   return true;
                                               }
                                               return false;
                                           }),
                            m_aAppliedEffects.end());
}

void
Unit::UpdateCDs(std::chrono::microseconds delta)
{
    for(auto& cd : m_aSpellCDs)
    {
        if(std::get<0>(cd) == false)
        {
            std::get<1>(cd) -= delta;
            
            if(std::get<1>(cd) <= 0s)
            {
                std::get<0>(cd) = true;
                std::get<1>(cd) = 0s;
            }
        }
    }
}

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
        // Log item drop event
    auto m_pLogSystem = m_poGameWorld->m_pLogSystem;
    auto& m_oLogBuilder = m_poGameWorld->m_oLogBuilder;
    m_oLogBuilder << this->GetName() << " TOOK ITEM " << (int)item->GetType();
    m_pLogSystem->Info(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
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
    m_pLogSystem->Info(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
    m_eState = Unit::State::WALKING;
    m_nObjAttributes = GameObject::Attributes::MOVABLE |
                        GameObject::Attributes::VISIBLE |
                        GameObject::Attributes::DAMAGABLE;
    m_nUnitAttributes = Unit::Attributes::INPUT |
    Unit::Attributes::ATTACK |
    Unit::Attributes::DUELABLE;
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
    m_pLogSystem->Info(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
    m_eState = Unit::State::WALKING;
    m_nObjAttributes = GameObject::Attributes::MOVABLE |
    GameObject::Attributes::VISIBLE |
    GameObject::Attributes::DAMAGABLE;
    m_nUnitAttributes = Unit::Attributes::INPUT |
    Unit::Attributes::ATTACK |
    Unit::Attributes::DUELABLE;
    m_nHealth = m_nMHealth;
    
    m_stLogPosition = log_pos;
    
    RespawnInvulnerability * pRespInv = new RespawnInvulnerability(5s);
    pRespInv->SetTargetUnit(this);
    this->ApplyEffect(pRespInv);
    
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
Unit::DropItem(int32_t index)
{
    auto item_iter = m_aInventory.begin() + index;
    
        // Log item drop event
    auto m_pLogSystem = m_poGameWorld->m_pLogSystem;
    auto& m_oLogBuilder = m_poGameWorld->m_oLogBuilder;
    m_oLogBuilder << this->GetName() << " DROPPED ITEM " << (int)(*item_iter)->GetType();
    m_pLogSystem->Info(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
        // make item visible and set its coords
    (*item_iter)->SetCarrierID(0);
    (*item_iter)->SetLogicalPosition(this->GetLogicalPosition());
    
        // delete item from inventory
    m_aInventory.erase(item_iter);
}

void
Unit::Die(Unit * killer)
{
    if(killer->GetDuelTarget() == this)
    {
        killer->EndDuel();
    }
        // Log dead event
    auto m_pLogSystem = m_poGameWorld->m_pLogSystem;
    auto& m_oLogBuilder = m_poGameWorld->m_oLogBuilder;
    m_oLogBuilder << this->GetName() << " KILLED BY " << killer->GetName() << " DIED AT (" << m_stLogPosition.x << ";" << m_stLogPosition.y << ")";
    m_pLogSystem->Info(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
        // drop items
    while(!m_aInventory.empty())
    {
        this->DropItem(0);
    }
    
    if(m_pDuelTarget != nullptr)
        EndDuel();
    
    flatbuffers::FlatBufferBuilder builder;
    auto move = GameEvent::CreateSVActionDeath(builder,
                                               this->GetUID(),
                                               killer->GetUID());
    auto msg = GameEvent::CreateMessage(builder,
                                        GameEvent::Events_SVActionDeath,
                                        move.Union());
    builder.Finish(msg);
    m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
    
    m_eState = Unit::State::DEAD;
    m_nObjAttributes = GameObject::Attributes::PASSABLE;
    m_nUnitAttributes = 0;
    m_nHealth = 0;
    m_poGameWorld->m_aRespawnQueue.push_back(std::make_pair(3s, this));
}

void
Unit::Move(MoveDirection dir)
{
    Point2 new_coord = this->GetLogicalPosition();
    if(dir == MoveDirection::LEFT)
        --new_coord.x;
    else if(dir == MoveDirection::RIGHT)
        ++new_coord.x;
    else if(dir == MoveDirection::UP)
        ++new_coord.y;
    else if(dir == MoveDirection::DOWN)
        --new_coord.y;
    
        // firstly - check if it can go there (no unpassable objects)
    for(auto object : m_poGameWorld->m_apoObjects)
    {
        if(object->GetLogicalPosition() == new_coord &&
           !(object->GetAttributes() & GameObject::Attributes::PASSABLE))
        {
            return; // there is an unpassable object
        }
    }
        // Log move event
    auto m_pLogSystem = m_poGameWorld->m_pLogSystem;
    auto& m_oLogBuilder = m_poGameWorld->m_oLogBuilder;
    m_oLogBuilder << this->GetName() << " MOVE (" << m_stLogPosition.x
    << ";" << m_stLogPosition.y << ") -> (" << new_coord.x << ";" << new_coord.y << ")";
    m_pLogSystem->Info(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
    this->SetLogicalPosition(new_coord);
    
    flatbuffers::FlatBufferBuilder builder;
    auto move = GameEvent::CreateSVActionMove(builder,
                                              this->GetUID(),
                                              (char)dir,
                                              new_coord.x,
                                              new_coord.y);
    auto msg = GameEvent::CreateMessage(builder,
                                        GameEvent::Events_SVActionMove,
                                        move.Union());
    builder.Finish(msg);
    m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
}

void
Unit::TakeDamage(int16_t damage,
                 Unit::DamageType dmg_type,
                 Unit * damage_dealer)
{
    int16_t damage_taken = damage;
    if(dmg_type == Unit::DamageType::PHYSICAL)
        damage_taken -= m_nArmor;
    else if(dmg_type == Unit::DamageType::MAGICAL)
        damage_taken -= m_nMResistance;
    
    m_nHealth -= damage_taken;
    
        // Log damage take event
    auto m_pLogSystem = m_poGameWorld->m_pLogSystem;
    auto& m_oLogBuilder = m_poGameWorld->m_oLogBuilder;
    m_oLogBuilder << this->GetName() << " TOOK " << damage_taken << " FROM " << damage_dealer->GetName()
    << " HP " << m_nHealth + damage_taken << "->" << m_nHealth;
    m_pLogSystem->Info(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
    if(m_nHealth <= 0)
    {
        Die(damage_dealer);
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
    m_pLogSystem->Info(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
    m_eState = Unit::State::DUEL;
    m_nUnitAttributes &= ~Unit::Attributes::DUELABLE;
    
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
    m_pLogSystem->Info(m_oLogBuilder.str());
    m_oLogBuilder.str("");
    
    m_eState = Unit::State::WALKING;
    m_nUnitAttributes |=
    Unit::Attributes::INPUT |
    Unit::Attributes::ATTACK |
    Unit::Attributes::DUELABLE;
    
    m_pDuelTarget = nullptr;
}
