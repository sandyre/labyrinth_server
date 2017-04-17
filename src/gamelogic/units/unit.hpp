//
//  unit.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef unit_hpp
#define unit_hpp

#include "../gameobject.hpp"
#include "../item.hpp"

#include <string>
#include <vector>

class Unit : public GameObject
{
public:
    enum class Type
    {
        UNDEFINED,
        MONSTER,
        HERO
    };
    enum class Orientation
    {
        UP,
        DOWN,
        LEFT,
        RIGHT
    };
    enum class State
    {
        UNDEFINED,
        WALKING,
        SWAMP,
        DUEL,
        DEAD
    };
public:
    Unit::Type          GetUnitType() const;
    Unit::State         GetState() const;
    Unit::Orientation   GetOrientation() const;
    
    std::string         GetName() const;
    void                SetName(const std::string&);
    
    int16_t             GetDamage() const;
    int16_t             GetHealth() const;
    int16_t             GetMaxHealth() const;
    
    Unit * const        GetDuelTarget() const;
    std::vector<Item*>& GetInventory();
    virtual void        UpdateStats(); // based on inventory

    virtual void    Spawn(Point2);
    virtual void    Respawn(Point2);
    virtual void    Move(Point2);
    virtual void    TakeItem(Item*);
    
    virtual void    StartDuel(Unit*);
    virtual void    Attack();
    virtual void    TakeDamage(int16_t);
    virtual void    EndDuel();
    
    virtual void    Die();
protected:
    Unit();
    
protected:
    Unit::Type          m_eUnitType;
    Unit::State         m_eState;
    Unit::Orientation   m_eOrientation;
    
    std::string         m_sName;
    
    int16_t             m_nBaseDamage;
    int16_t             m_nActualDamage;
    int16_t             m_nHealth, m_nMHealth;
    
    float   m_nMoveSpeed;
    
    std::vector<Item*>  m_aInventory;
        // Duel-data
    Unit *              m_pDuelTarget;
};

#endif /* unit_hpp */
