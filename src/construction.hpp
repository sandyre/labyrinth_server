//
//  construction.hpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 02.02.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef construction_hpp
#define construction_hpp

struct Construction
{
    enum class Type
    {
        DOOR = 0x00,
        GRAVEYARD
    };
    
    Construction::Type  eType;
    uint16_t            nXCoord;
    uint16_t            nYCoord;
};

#endif /* construction_hpp */
