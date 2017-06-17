//
//  item.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "item.hpp"

Item::Item() :
_carrierUID(0)
{
    _objType = GameObject::Type::ITEM;
    _objAttributes |= GameObject::Attributes::PASSABLE;
}

Item::Type
Item::GetType() const
{
    return _itemType;
}

uint32_t
Item::GetCarrierID() const
{
    return _carrierUID;
}

void
Item::SetCarrierID(uint32_t id)
{
    _carrierUID = id;
}

Key::Key()
{
    _itemType = Item::Type::KEY;
}

Sword::Sword()
{
    _itemType = Item::Type::SWORD;
}
