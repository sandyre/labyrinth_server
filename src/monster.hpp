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
    enum State
    {
        WAITING,
        CHARGING,
        DUEL,
        DEAD
    };
    
    State  eState = WAITING;
    Point2 stPosition;
    uint16_t nUID;
    uint32_t nChargingUID;
    uint32_t nTimer = 0;
    uint32_t nAttackCD = 4000;
    uint32_t nMoveCD = 1000; // in ms
    uint8_t nHP = 3;
    uint8_t nMaxHP = 3;
    uint8_t nDamage = 1;
    uint8_t nVision = 3;
    uint8_t nChargeRadius = 8;
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
        monster.nChargingUID = 0;
        
        ++m_nCurrentID;
        return monster;
    }
protected:
    uint16_t m_nCurrentID;
};

#endif /* monster_hpp */
