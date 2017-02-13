//
//  item.hpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 02.02.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef item_hpp
#define item_hpp

#include "globals.h"

struct Item
{
    enum Type : unsigned char
    {
        KEY = 0x00,
        SWORD = 0x01
    };
    
    Item::Type   eType;
    uint16_t     nUID;
    Point2       stPosition;
    uint32_t     nCarrierID; // 0 means its not carried by any player
};

#endif /* item_hpp */
