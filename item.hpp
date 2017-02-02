//
//  item.hpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 02.02.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef item_hpp
#define item_hpp

struct Item
{
    enum class Type : unsigned char
    {
        KEY = 0x00,
        SWORD = 0x01
    };
    
    Item::Type  eType;
};

#endif /* item_hpp */
