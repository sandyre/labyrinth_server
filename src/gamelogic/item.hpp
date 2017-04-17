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
    
    Item::Type  GetType() const;
    
    uint32_t    GetCarrierID() const;
    void        SetCarrierID(uint32_t);
    
protected:
    Item();
    
    Item::Type  m_eType;
    uint32_t    m_nCarrierID;
};

class Key : public Item
{
public:
    Key();
};

class Sword : public Item
{
public:
    Sword();
};

#endif /* item_hpp */
