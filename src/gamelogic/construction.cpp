//
//  construction.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "construction.hpp"

Construction::Construction()
{
    _objType = GameObject::Type::CONSTRUCTION;
}

Door::Door()
{
    _constrType = Construction::Type::DOOR;
    _objAttributes |= GameObject::Attributes::PASSABLE;
}

Graveyard::Graveyard()
{
    _constrType = Construction::Type::GRAVEYARD;
    _objAttributes |= GameObject::Attributes::PASSABLE;
}

Swamp::Swamp()
{
    _constrType = Construction::Type::SWAMP;
}
