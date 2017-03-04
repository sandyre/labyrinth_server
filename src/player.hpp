//
//  player.hpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef player_hpp
#define player_hpp

#include <Poco/Net/SocketAddress.h>
#include <vector>
#include "item.hpp"
#include "globals.h"

struct Player
{
    enum Hero
    {
        ROGUE,
        PALADIN,
        UNDEFINED
    };
    enum State : unsigned char
    {
        PRE_UNDEFINED,
        PRE_CONNECTED_TO_SERVER,
        PRE_READY_TO_START,
        
        IN_WALKING,
        IN_DUEL,
        IN_SWAMP,
        IN_INVULNERABLE,
        IN_DEAD
    };
    
    State       eState = PRE_UNDEFINED;
    Hero        eHero = UNDEFINED;
    Point2      stPosition;
    uint32_t    nTimer = 0;
    uint32_t    nUID = 0;
    char        sNickname[16];
    int16_t     nHP = 40;
    int16_t     nHPMax = 40;
    uint16_t    nDamage = 1;
    
    Poco::Net::SocketAddress sock_addr;
};

#endif /* player_hpp */
