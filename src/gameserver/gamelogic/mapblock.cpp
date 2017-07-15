//
//  mapblock.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "mapblock.hpp"

MapBlock::MapBlock(GameWorld& world)
: GameObject(world)
{
    _objType = GameObject::Type::MAPBLOCK;
}

MapBlock::Type
MapBlock::GetType() const
{
    return _blockType;
}

NoBlock::NoBlock(GameWorld& world)
: MapBlock(world)
{
    _blockType = MapBlock::Type::NOBLOCK;
    _objAttributes |= GameObject::Attributes::PASSABLE;
}

WallBlock::WallBlock(GameWorld& world)
: MapBlock(world)
{
    _blockType = MapBlock::Type::WALL;
}

BorderBlock::BorderBlock(GameWorld& world)
: MapBlock(world)
{
    _blockType = MapBlock::Type::BORDER;
}
