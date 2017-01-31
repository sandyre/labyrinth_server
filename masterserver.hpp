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
#include <Poco/Net/DatagramSocket.h>

#include "player.hpp"
#include "gameserver.hpp"

class MasterServer
{
public:
    MasterServer(unsigned int Port);
    ~MasterServer();
    
    void    run();
protected:
    std::vector<Player>       m_aPlayersPool;
    std::vector<GameServer*>  m_aGameServers;
    Poco::Net::DatagramSocket m_oSocket;
};

#endif /* masterserver_hpp */
