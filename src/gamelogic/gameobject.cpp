//
//  gameobject.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "gameobject.hpp"

GameObject::GameObject() :
_objType(GameObject::Type::UNDEFINED),
_UID(0),
_objAttributes(0)
{
    _objAttributes |= GameObject::Attributes::VISIBLE;
}

GameObject::~GameObject()
{
    
}

GameObject::Type
GameObject::GetObjType() const
{
    return _objType;
}

uint32_t
GameObject::GetAttributes() const
{
    return _objAttributes;
}

void
GameObject::SetGameWorld(GameWorld * _gameworld)
{
    _gameWorld = _gameworld;
}

uint32_t
GameObject::GetUID() const
{
    return _UID;
}

void
GameObject::SetUID(uint32_t val)
{
    _UID = val;
}

Point2
GameObject::GetLogicalPosition() const
{
    return _logPos;
}

void
GameObject::SetLogicalPosition(Point2 pos)
{
    _logPos = pos;
}
