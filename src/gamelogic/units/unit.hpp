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

class Mage;
class Warrior;

class Effect;
class WarriorDash;
class WarriorArmorUp;
class RogueInvisibility;
class MageFreeze;
class DuelInvulnerability;
class RespawnInvulnerability;

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
    enum class MoveDirection
    {
        UP = 0x00,
        DOWN = 0x01,
        LEFT = 0x02,
        RIGHT = 0x03
    };
    enum class State
    {
        UNDEFINED,
        WALKING,
        SWAMP,
        DUEL,
        DEAD
    };
    enum class DamageType
    {
        PHYSICAL = 0x00,
        MAGICAL = 0x01
    };
    struct Attributes
    {
        static const int INPUT = 0x01;
        static const int ATTACK = 0x02;
        static const int DUELABLE = 0x04;
    };
public:
    Unit::Type          GetUnitType() const;
    Unit::State         GetState() const;
    Unit::Orientation   GetOrientation() const;
    uint32_t            GetUnitAttributes() const;
    
    std::string         GetName() const;
    void                SetName(const std::string&);
    
    int16_t             GetDamage() const;
    int16_t             GetHealth() const;
    int16_t             GetMaxHealth() const;
    int16_t             GetArmor() const;
    
    Unit * const        GetDuelTarget() const;
    std::vector<Item*>& GetInventory();
    virtual void        UpdateStats(); // based on inventory

    virtual void    TakeDamage(int16_t, DamageType, Unit *);
    virtual void    Spawn(Point2);
    virtual void    Respawn(Point2);
    virtual void    Move(MoveDirection);
    virtual void    TakeItem(Item*);
    virtual void    DropItem(int32_t index);
    
    virtual void    StartDuel(Unit*);
    virtual void    EndDuel();
    
    virtual void    Die(Unit *);
    
        // additional funcs
    virtual void    ApplyEffect(Effect*);
protected:
    Unit();
    
    virtual void        update(std::chrono::microseconds) override;
    virtual void        UpdateCDs(std::chrono::microseconds);
protected:
    Unit::Type          m_eUnitType;
    Unit::State         m_eState;
    Unit::Orientation   m_eOrientation;
    
    std::string         m_sName;
    
    uint32_t            m_nUnitAttributes;
    
    int16_t             m_nBaseDamage;
    int16_t             m_nActualDamage;
    int16_t             m_nHealth, m_nMHealth;
    int16_t             m_nArmor;
    int16_t             m_nMResistance;
    
    float   m_nMoveSpeed;
    
    std::vector<std::tuple<bool, std::chrono::microseconds, std::chrono::microseconds>> m_aSpellCDs;
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
    friend RespawnInvulnerability;
    
    friend Mage;
    friend Warrior;
};

#endif /* unit_hpp */
