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

Construction::Type
Construction::GetType() const
{
    return m_eType;
}

Door::Door()
{
    m_eType = Construction::Type::DOOR;
}

Graveyard::Graveyard()
{
    m_eType = Construction::Type::GRAVEYARD;
}

Swamp::Swamp()
{
    m_eType = Construction::Type::SWAMP;
}
