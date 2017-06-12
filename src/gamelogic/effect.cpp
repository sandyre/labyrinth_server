//
//  effect.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 12.05.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "effect.hpp"
#include "units/warrior.hpp"

using namespace std::chrono_literals;

Effect::Effect() :
m_nADuration(0),
m_eEffState(Effect::State::ACTIVE)
{
    
}

Effect::~Effect()
{
    
}

Effect::State
Effect::GetState() const
{
    return m_eEffState;
}

void
Effect::SetTargetUnit(Unit * target)
{
    m_pTargetUnit = target;
}

WarriorDash::WarriorDash(std::chrono::microseconds duration,
                         float bonus_movespeed) :
m_nBonusMovespeed(bonus_movespeed)
{
    m_nADuration = duration;
}

void
WarriorDash::start()
{
    m_pTargetUnit->m_nMoveSpeed -= m_nBonusMovespeed;
}

void
WarriorDash::update(std::chrono::microseconds delta)
{
    if(m_eEffState == Effect::State::ACTIVE)
    {
        m_nADuration -= delta;
        if(m_nADuration < 0s)
        {
            m_eEffState = Effect::State::OVER;
        }
    }
}

void
WarriorDash::stop()
{
    m_pTargetUnit->m_nMoveSpeed += m_nBonusMovespeed;
}

void
WarriorArmorUp::start()
{
    m_pTargetUnit->m_nArmor += m_nBonusArmor;
}

WarriorArmorUp::WarriorArmorUp(std::chrono::microseconds duration,
                               int16_t bonus_armor) :
m_nBonusArmor(bonus_armor)
{
    m_nADuration = duration;
}

void
WarriorArmorUp::update(std::chrono::microseconds delta)
{
    if(m_eEffState == Effect::State::ACTIVE)
    {
        m_nADuration -= delta;
        if(m_nADuration < 0s)
        {
            m_eEffState = Effect::State::OVER;
        }
    }
}

void
WarriorArmorUp::stop()
{
    m_pTargetUnit->m_nArmor -= m_nBonusArmor;
}

RogueInvisibility::RogueInvisibility(std::chrono::microseconds duration)
{
    m_nADuration = duration;
}

void
RogueInvisibility::start()
{
    m_pTargetUnit->m_nObjAttributes &= ~(GameObject::Attributes::VISIBLE);
    m_pTargetUnit->m_nUnitAttributes &= ~(Unit::Attributes::DUELABLE);}

void
RogueInvisibility::update(std::chrono::microseconds delta)
{
    if(m_eEffState == Effect::State::ACTIVE)
    {
        m_nADuration -= delta;
        if(m_nADuration < 0s)
        {
            m_eEffState = Effect::State::OVER;
        }
    }
}

void
RogueInvisibility::stop()
{
    m_pTargetUnit->m_nObjAttributes |= (GameObject::Attributes::VISIBLE);
    m_pTargetUnit->m_nUnitAttributes |= (Unit::Attributes::DUELABLE);
}

MageFreeze::MageFreeze(std::chrono::microseconds duration)
{
    m_nADuration = duration;
}

void
MageFreeze::start()
{
    m_pTargetUnit->m_nUnitAttributes &= ~(Unit::Attributes::INPUT);
}

void
MageFreeze::update(std::chrono::microseconds delta)
{
    if(m_eEffState == Effect::State::ACTIVE)
    {
        m_nADuration -= delta;
        if(m_nADuration < 0s)
        {
            m_eEffState = Effect::State::OVER;
        }
    }
}

void
MageFreeze::stop()
{
    m_pTargetUnit->m_nUnitAttributes |= Unit::Attributes::INPUT;
}

DuelInvulnerability::DuelInvulnerability(std::chrono::microseconds duration)
{
    m_nADuration = duration;
}

void
DuelInvulnerability::start()
{
    m_pTargetUnit->m_nUnitAttributes &= ~(Unit::Attributes::DUELABLE);}

void
DuelInvulnerability::update(std::chrono::microseconds delta)
{
    if(m_eEffState == Effect::State::ACTIVE)
    {
        m_nADuration -= delta;
        if(m_nADuration < 0s)
        {
            m_eEffState = Effect::State::OVER;
        }
    }
}

void
DuelInvulnerability::stop()
{
    m_pTargetUnit->m_nUnitAttributes |= Unit::Attributes::DUELABLE;
}

RespawnInvulnerability::RespawnInvulnerability(std::chrono::microseconds duration)
{
    m_nADuration = duration;
}

void
RespawnInvulnerability::start()
{
    m_pTargetUnit->m_nUnitAttributes &= ~(Unit::Attributes::DUELABLE);
    m_pTargetUnit->m_nObjAttributes &= ~(GameObject::Attributes::PASSABLE);
}

void
RespawnInvulnerability::update(std::chrono::microseconds delta)
{
    if(m_eEffState == Effect::State::ACTIVE)
    {
        m_nADuration -= delta;
        if(m_nADuration < 0s)
        {
            m_eEffState = Effect::State::OVER;
        }
    }
}

void
RespawnInvulnerability::stop()
{
    m_pTargetUnit->m_nUnitAttributes |= Unit::Attributes::DUELABLE;
    m_pTargetUnit->m_nObjAttributes |= ~(GameObject::Attributes::PASSABLE);
}
