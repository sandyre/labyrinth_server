//
//  hero.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 05.03.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef hero_hpp
#define hero_hpp

#include "globals.h"
#include "item.hpp"

#include <chrono>
#include <vector>

class Hero
{
public:
    enum Type
    {
        FIRST_HERO = 0x00,
        AIR_ELEMENTALIST = 0x00,
        WATER_ELEMENTALIST = 0x01,
        FIRE_ELEMENTALIST = 0x02,
        EARTH_ELEMENTALIST = 0x03,
        LAST_HERO = 0x03
    };
    enum class State
    {
        WALKING,
        DUEL_HERO,
        DUEL_MONSTER,
        SWAMP,
        DEAD,
        REQUEST_RESPAWN
    };
    struct Status
    {
        static const int DEFAULT = 0x00; // unmovable/unduelable/etc
        static const int MOVABLE = 0x01;
        static const int DUELABLE = 0x02;
        static const int INVULNERABLE = 0x04;
    };
public:
    Hero()
    {
        m_eHero = Hero::Type::FIRST_HERO;
        m_nStatus = Status::MOVABLE | Status::DUELABLE;
        
        m_nDamage = m_nHealth = m_nMHealth = 0;
        
        m_nDuelTargetUID = 0;
    }
    
    Hero::Type  GetHeroType() const
    {
        return m_eHero;
    }
    
    int32_t     GetStatus() const
    {
        return m_nStatus;
    }
    
    Hero::State GetState() const
    {
        return m_eState;
    }
    
    void        SetState(Hero::State state)
    {
        m_eState = state;
    }
    
    int16_t     GetHealth() const
    {
        return m_nHealth;
    }
    void        SetHealth(int16_t hp)
    {
        m_nHealth = hp;
    }
    
    int16_t     GetMHealth() const
    {
        return m_nMHealth;
    }
    
    int16_t     GetDamage() const
    {
        return m_nDamage;
    }
    
    Point2      GetPosition() const
    {
        return m_stPosition;
    }
    void        SetPosition(const Point2& pos)
    {
        m_stPosition = pos;
    }
    void        SetPosition(uint16_t x,
                            uint16_t y)
    {
        m_stPosition.x = x;
        m_stPosition.y = y;
    }
    
    virtual void    Update(std::chrono::milliseconds delta)
    {
        if(m_eState == State::DEAD &&
           m_nRespawnTimer > 0.0)
        {
            m_nRespawnTimer -= delta.count();
        }
        else if(m_eState == State::DEAD &&
                m_nRespawnTimer <= 0.0)
        {
            m_eState = Hero::State::REQUEST_RESPAWN;
            m_nRespawnTimer = 0.0;
        }
    }
    
    virtual void    Respawn(const Point2& pos)
    {
        m_eState = State::WALKING;
        m_nStatus = Hero::Status::MOVABLE | Hero::Status::DUELABLE;
        m_stPosition = pos;
        m_nHealth = m_nMHealth;
    }
    virtual void    MoveTo(const Point2& pos)
    {
        m_stPosition = pos;
    }

    virtual void    TakeItem(const Item& item)
    {
        m_aInventory.push_back(item);
    }
    virtual void    Die(float nRespawnTime)
    {
        m_eState = Hero::State::DEAD;
        m_nStatus = Hero::Status::DEFAULT;
        m_nRespawnTimer = nRespawnTime;
    }
    virtual void    StartDuel(uint32_t uid,
                              bool bPlayer)
    {
        if(bPlayer)
            m_eState = Hero::State::DUEL_HERO;
        else
            m_eState = Hero::State::DUEL_MONSTER;
        
        m_nStatus ^= Hero::Status::DUELABLE | Hero::Status::MOVABLE;
        m_nDuelTargetUID = uid;
    }
    virtual void    EndDuel()
    {
        m_eState = Hero::State::WALKING;
        
        m_nDuelTargetUID = 0;
        m_nStatus = Hero::Status::DUELABLE | Hero::Status::MOVABLE;
    }
    virtual void    SpellCast1() = 0;
protected:
    Hero::Type  m_eHero;
    int32_t     m_nStatus;
    Hero::State m_eState;
    
    uint32_t    m_nDuelTargetUID;
    
    Point2      m_stPosition;
    
    int16_t     m_nHealth;
    int16_t     m_nMHealth;
    
    int16_t     m_nDamage;
    
    float       m_nRespawnTimer;
    
    std::vector<Item>   m_aInventory;
};

class EarthElementalist : public Hero
{
public:
    EarthElementalist()
    {
        m_eHero = Hero::Type::EARTH_ELEMENTALIST;
        
        m_nHealth = m_nMHealth = 100;
        m_nDamage = 8;
    }
    
    virtual void    SpellCast1() override
    {
        
    }
};

class FireElementalist : public Hero
{
public:
    FireElementalist()
    {
        m_eHero = Hero::Type::FIRE_ELEMENTALIST;
        
        m_nHealth = m_nMHealth = 75;
        m_nDamage = 10;
    }
    
    virtual void    SpellCast1() override
    {
        
    }
};

class WaterElementalist : public Hero
{
public:
    WaterElementalist()
    {
        m_eHero = Hero::Type::WATER_ELEMENTALIST;
        
        m_nHealth = m_nMHealth = 80;
        m_nDamage = 8;
    }
    
    virtual void    SpellCast1() override
    {
        
    }
};

class AirElementalist : public Hero
{
public:
    AirElementalist()
    {
        m_eHero = Hero::Type::AIR_ELEMENTALIST;
        
        m_nHealth = m_nMHealth = 60;
        m_nDamage = 8;
        
        m_nInvisDuration = 10000;
        m_nInvisADuration = 0;
        m_bInvisible = false;
    }
    
    virtual void    TakeItem(const Item& item) override
    {
        m_aInventory.push_back(item);
        
        if(m_bInvisible)
        {
            m_bInvisible = false;
            m_nInvisADuration = 0.0;
            m_nStatus |= Hero::Status::DUELABLE;
        }
    }
    virtual void    SpellCast1() override
    {
        m_nInvisADuration = m_nInvisDuration;
        m_bInvisible = true;
        
        m_nStatus ^= Hero::Status::DUELABLE;
    }
    
    virtual void    Update(std::chrono::milliseconds delta) override
    {
        Hero::Update(delta); // updates respawn timer
        
        if(m_bInvisible &&
           m_nInvisADuration > 0)
        {
            m_nInvisADuration -= delta.count();
        }
        else if(m_bInvisible &&
                m_nInvisADuration <= 0)
        {
            m_bInvisible = false;
            m_nInvisADuration = 0;
            m_nStatus |= Hero::Status::DUELABLE;
        }
    }
protected:
    bool    m_bInvisible;
    int32_t   m_nInvisADuration;
    int32_t   m_nInvisDuration;
};

#endif /* hero_hpp */
