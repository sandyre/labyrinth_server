//
//  mapblock.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "mapblock.hpp"

MapBlock::MapBlock()
{
    m_eObjType = GameObject::Type::MAPBLOCK;
}

MapBlock::Type
MapBlock::GetType() const
{
    return m_eType;
}

NoBlock::NoBlock()
{
    m_eType = MapBlock::Type::NOBLOCK;
    m_nObjAttributes |= GameObject::Attributes::PASSABLE;
}

WallBlock::WallBlock()
{
    m_eType = MapBlock::Type::WALL;
}

BorderBlock::BorderBlock()
{
    m_eType = MapBlock::Type::BORDER;
}
