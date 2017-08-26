//
//  construction.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "construction.hpp"


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


Swamp::Swamp(GameWorld& world, uint32_t uid)
: Construction(world, uid)
{
    _constrType = Construction::Type::SWAMP;
}
