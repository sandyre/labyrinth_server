//
//  effect.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 12.05.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef effect_hpp
#define effect_hpp

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>


class Unit;

class Effect
{
public:
    enum State
    {
        ACTIVE = 0x00,
        OVER = 0x01,
        PAUSED = 0x02
    };
    
public:
    Effect();
    virtual ~Effect() { }

    State GetState() const
    { return _state; }
    const std::string& GetName() const
    { return _name; }
    
    void SetTargetUnit(std::shared_ptr<Unit> target)
    { _targetUnit = target; }
    
    virtual void start() = 0;
    virtual void update(std::chrono::microseconds) = 0; // called each frame by unit its applied to
    virtual void stop() = 0;

protected:
    State                       _state;
    std::chrono::microseconds   _timer;
    std::weak_ptr<Unit>         _targetUnit;
    std::string                 _name;
};


class WarriorDash : public Effect
{
public:
    WarriorDash(std::chrono::microseconds duration, float bonus_movespeed);

    virtual void start() override;
    virtual void update(std::chrono::microseconds) override;
    virtual void stop() override;
protected:
    float _bonusSpeed;
};


class WarriorArmorUp : public Effect
{
public:
    WarriorArmorUp(std::chrono::microseconds duration, int16_t bonus_armor);

    virtual void start() override;
    virtual void update(std::chrono::microseconds) override;
    virtual void stop() override;
protected:
    int16_t _bonusArmor;
};


class RogueInvisibility : public Effect
{
public:
    RogueInvisibility(std::chrono::microseconds duration);

    virtual void start() override;
    virtual void update(std::chrono::microseconds) override;
    virtual void stop() override;
};


class MageFreeze : public Effect
{
public:
    MageFreeze(std::chrono::microseconds duration);

    virtual void start() override;
    virtual void update(std::chrono::microseconds) override;
    virtual void stop() override;
};


class DuelInvulnerability : public Effect
{
public:
    DuelInvulnerability(std::chrono::microseconds duration);

    virtual void start() override;
    virtual void update(std::chrono::microseconds) override;
    virtual void stop() override;
};


class RespawnInvulnerability : public Effect
{
public:
    RespawnInvulnerability(std::chrono::microseconds duration);

    virtual void start() override;
    virtual void update(std::chrono::microseconds) override;
    virtual void stop() override;
};


class FountainHeal : public Effect
{
public:
    FountainHeal(std::chrono::microseconds duration);

    virtual void start() override;
    virtual void update(std::chrono::microseconds) override;
    virtual void stop() override;
};

#endif /* effect_hpp */
