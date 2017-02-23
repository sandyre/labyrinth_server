//
//  monster.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 21.02.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef monster_hpp
#define monster_hpp

#include "globals.h"

struct Monster
{
    Point2 stPosition;
    uint16_t nUID;
    uint8_t nHP = 3;
    uint8_t nMaxHP = 3;
    uint8_t nDamage = 1;
};

class MonsterFactory
{
public:
    MonsterFactory() :
    m_nCurrentID(0)
    {
        
    }
    
    Monster createMonster()
    {
        Monster monster;
        monster.nUID = m_nCurrentID;
        
        ++m_nCurrentID;
        return monster;
    }
protected:
    uint16_t m_nCurrentID;
};

#endif /* monster_hpp */
