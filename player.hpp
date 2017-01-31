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

struct Player
{
    unsigned int x, y;
    int32_t uid;
    Poco::Net::SocketAddress sock_addr;
};

#endif /* player_hpp */
