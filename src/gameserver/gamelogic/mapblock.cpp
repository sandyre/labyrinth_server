//
//  mapblock.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "mapblock.hpp"


MapBlock::MapBlock(GameWorld& world, uint32_t uid)
: GameObject(world, uid)
{
    _objType = GameObject::Type::MAPBLOCK;
}


MapBlock::Type
MapBlock::GetType() const
{
    return _blockType;
}


NoBlock::NoBlock(GameWorld& world, uint32_t uid)
: MapBlock(world, uid)
{
    _blockType = MapBlock::Type::NOBLOCK;
    _objAttributes |= GameObject::Attributes::PASSABLE;
}


WallBlock::WallBlock(GameWorld& world, uint32_t uid)
: MapBlock(world, uid)
{
    _blockType = MapBlock::Type::WALL;
}


BorderBlock::BorderBlock(GameWorld& world, uint32_t uid)
: MapBlock(world, uid)
{
    _blockType = MapBlock::Type::BORDER;
}
