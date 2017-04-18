//
//  gameobject.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "gameobject.hpp"

GameObject::GameObject() :
m_eObjType(GameObject::Type::UNDEFINED),
m_nUID(0),
m_nAttributes(0)
{
    m_nAttributes |= GameObject::Attributes::VISIBLE;
}

GameObject::~GameObject()
{
    
}

GameObject::Type
GameObject::GetObjType() const
{
    return m_eObjType;
}

uint32_t
GameObject::GetAttributes() const
{
    return m_nAttributes;
}

void
GameObject::SetGameWorld(GameWorld * _gameworld)
{
    m_poGameWorld = _gameworld;
}

uint32_t
GameObject::GetUID() const
{
    return m_nUID;
}

void
GameObject::SetUID(uint32_t val)
{
    m_nUID = val;
}

Point2
GameObject::GetLogicalPosition() const
{
    return m_stLogPosition;
}

void
GameObject::SetLogicalPosition(Point2 pos)
{
    m_stLogPosition = pos;
}
