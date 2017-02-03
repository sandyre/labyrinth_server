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

struct Player
{
    uint16_t    nXCoord;
    uint16_t    nYCoord;
    uint32_t    nUID;
    
    Poco::Net::SocketAddress sock_addr;
};

#endif /* player_hpp */
