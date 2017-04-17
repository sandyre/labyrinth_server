//
//  item.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "item.hpp"

Item::Item() :
m_nCarrierID(0)
{
    m_eObjType = GameObject::Type::ITEM;
    m_nAttributes |= GameObject::Attributes::PASSABLE;
}

Item::Type
Item::GetType() const
{
    return m_eType;
}

uint32_t
Item::GetCarrierID() const
{
    return m_nCarrierID;
}

void
Item::SetCarrierID(uint32_t id)
{
    m_nCarrierID = id;
}

Key::Key()
{
    m_eType = Item::Type::KEY;
}

Sword::Sword()
{
    m_eType = Item::Type::SWORD;
}
