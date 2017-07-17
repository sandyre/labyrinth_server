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
#include "../../GameMessage.h"

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
        UNDEFINED, MONSTER, HERO
    };

    enum class Orientation
    {
        UP, DOWN, LEFT, RIGHT
    };

    enum class MoveDirection
    {
        UP = 0x00, DOWN = 0x01, LEFT = 0x02, RIGHT = 0x03
    };

    enum class State
    {
        UNDEFINED, WALKING, SWAMP, DUEL, DEAD
    };

    struct Attributes
    {
        static const int INPUT    = 0x01;
        static const int ATTACK   = 0x02;
        static const int DUELABLE = 0x04;
    };

    struct DamageDescriptor
    {
        enum class DamageType
        {
            PHYSICAL = 0x00, MAGICAL = 0x01
        };

        DamageType  Type;
        uint16_t    Value;
        std::string DealerName;
    };

public:
    Unit::Type GetType() const
    { return _unitType; }

    Unit::State GetState() const
    { return _state; }

    Unit::Orientation GetOrientation() const
    { return _orientation; }

    uint32_t GetUnitAttributes() const
    { return _unitAttributes; }

    std::string GetName() const
    { return _name; }

    void SetName(const std::string& name)
    { _name = name; }

    int16_t GetDamage() const
    { return _actualDamage; }

    int16_t GetHealth() const
    { return _health; }

    int16_t GetMaxHealth() const
    { return _maxHealth; }

    int16_t GetArmor() const
    { return _armor; }

    int16_t GetResistance() const
    { return _magResistance; }

    std::vector<std::shared_ptr<Item>>& GetInventory()
    { return _inventory; }

    virtual void TakeDamage(const DamageDescriptor& dmg);
    virtual void Spawn(Point<>);
    virtual void Respawn(Point<>);
    virtual void Move(MoveDirection);
    virtual void TakeItem(std::shared_ptr<Item> enemy);
    virtual std::shared_ptr<Item> DropItem(int32_t uid);

    virtual void SpellCast(const GameMessage::CLActionSpell*) = 0;

    virtual void StartDuel(std::shared_ptr<Unit> enemy);
    virtual void EndDuel();

    virtual void Die(const std::string& killerName);

    // additional funcs
    virtual void ApplyEffect(std::shared_ptr<Effect> effect);

protected:
    Unit(GameWorld& world);

    virtual void update(std::chrono::microseconds) override;
    virtual void UpdateCDs(std::chrono::microseconds);
    void UpdateStats();
    
protected:
    Unit::Type        _unitType;
    Unit::State       _state;
    Unit::Orientation _orientation;
    std::string       _name;
    uint32_t          _unitAttributes;
    int16_t           _baseDamage;
    int16_t           _actualDamage;
    int16_t           _health, _maxHealth;
    int16_t           _armor;
    int16_t           _magResistance;
    float             _moveSpeed;

    std::vector<std::tuple<bool, std::chrono::microseconds, std::chrono::microseconds>> _spellsCDs;
    std::vector<std::shared_ptr<Item>>                                                  _inventory;
    // Duel-data
    std::shared_ptr<Unit> _duelTarget;

    std::vector<std::shared_ptr<Effect>> _appliedEffects;

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
