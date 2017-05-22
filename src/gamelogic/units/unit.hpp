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
#include "../../gsnet_generated.h"

#include <string>
#include <vector>

class Effect;
class WarriorDash;
class WarriorArmorUp;
class RogueInvisibility;
class MageFreeze;
class DuelInvulnerability;

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
    int16_t             GetArmor() const;
    
    Unit * const        GetDuelTarget() const;
    std::vector<Item*>& GetInventory();
    virtual void        UpdateStats(); // based on inventory

    virtual void    Spawn(Point2);
    virtual void    Respawn(Point2);
    virtual void    Move(Point2);
    virtual void    TakeItem(Item*);
    virtual void    DropItem(int32_t index);
    
    virtual void    StartDuel(Unit*);
    virtual void    Attack(const GameEvent::CLActionAttack*);
    virtual void    TakeDamage(int16_t, uint8_t);
    virtual void    EndDuel();
    
    virtual void    Die();
    
        // additional funcs
    virtual void    ApplyEffect(Effect*);
protected:
    Unit();
    
    virtual void update(std::chrono::milliseconds) override;
protected:
    Unit::Type          m_eUnitType;
    Unit::State         m_eState;
    Unit::Orientation   m_eOrientation;
    
    std::string         m_sName;
    
    int16_t             m_nBaseDamage;
    int16_t             m_nActualDamage;
    int16_t             m_nHealth, m_nMHealth;
    int16_t             m_nArmor;
    int16_t             m_nMResistance;
    
    float   m_nMoveSpeed;
    
    std::vector<Item*>  m_aInventory;
        // Duel-data
    Unit *              m_pDuelTarget;
    
    std::vector<Effect*> m_aAppliedEffects;
        // Effects should have access to every field
    friend WarriorDash;
    friend WarriorArmorUp;
    friend RogueInvisibility;
    friend MageFreeze;
    friend DuelInvulnerability;
};

#endif /* unit_hpp */
