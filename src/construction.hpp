//
//  construction.hpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 02.02.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef construction_hpp
#define construction_hpp

#include "globals.h"

struct Construction
{
    enum Type : unsigned char
    {
        DOOR = 0x00,
        GRAVEYARD = 0x01,
        SWAMP = 0x02
    };
    
    Construction::Type  eType;
    uint16_t            nUID;
    Point2              stPosition;
};

class ConstructionFactory
{
public:
    ConstructionFactory() :
    m_nCurrentID(0)
    {
        
    }
    
    Construction createConstruction(Construction::Type type)
    {
        Construction constr;
        constr.eType = type;
        constr.nUID = m_nCurrentID;
        
        ++m_nCurrentID;
        return constr;
    }
private:
    uint16_t    m_nCurrentID;
};

#endif /* construction_hpp */
