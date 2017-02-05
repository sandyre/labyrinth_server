//
//  masterserver.hpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef masterserver_hpp
#define masterserver_hpp

#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <queue>
#include <Poco/Net/DatagramSocket.h>

#include "player.hpp"
#include "gameserver.hpp"

class MasterServer
{
public:
    MasterServer(uint32_t Port);
    ~MasterServer();
    
    void    run();
protected:
    std::deque<Player>        m_aPlayersPool;
    std::vector<GameServer*>  m_aGameServers;
    Poco::Net::DatagramSocket m_oSocket;
    
    uint32_t                  m_nCurrentGamePort;
};

#endif /* masterserver_hpp */
