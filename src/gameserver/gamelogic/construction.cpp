//
//  construction.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "construction.hpp"

#include "units/unit.hpp"
#include "effect.hpp"

using namespace std::chrono_literals;

Construction::Construction(GameWorld& world, uint32_t uid)
: GameObject(world, uid)
{
    _objType = GameObject::Type::CONSTRUCTION;
}


Door::Door(GameWorld& world, uint32_t uid)
: Construction(world, uid)
{
    _constrType = Construction::Type::DOOR;
}


Graveyard::Graveyard(GameWorld& world, uint32_t uid)
: Construction(world, uid)
{
    _constrType = Construction::Type::GRAVEYARD;
}


Fountain::Fountain(GameWorld& world, uint32_t uid)
: Construction(world, uid)
{
    _constrType = Construction::Type::FOUNTAIN;
    _objAttributes |= GameObject::Attributes::PASSABLE;
}


void
Fountain::OnCollision(const std::shared_ptr<GameObject>& object)
{
    if (const auto unit = std::dynamic_pointer_cast<Unit>(object);
        unit && _cooldown.Elapsed<std::chrono::microseconds>() > 30s)
    {
        unit->ApplyEffect(std::make_shared<FountainHeal>(5s));
        _cooldown.Reset();
    }
}
