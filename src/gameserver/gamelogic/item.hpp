//
//  item.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef item_hpp
#define item_hpp

#include "gameobject.hpp"


class Item : public GameObject
{
public:
    enum class Type
    {
        KEY,
        SWORD
    };

public:
    Item::Type  GetType() const;

    virtual void update(std::chrono::microseconds) override { }
    
protected:
    Item(GameWorld& world);
    
    Item::Type  _itemType;
    uint32_t    _carrierUID;
};


class Key : public Item
{
public:
    Key(GameWorld& world);
};


class Sword : public Item
{
public:
    Sword(GameWorld& world);
};

#endif /* item_hpp */
