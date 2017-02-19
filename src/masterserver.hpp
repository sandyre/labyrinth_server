//
//  masterserver.hpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef masterserver_hpp
#define masterserver_hpp

#include <thread>
#include <vector>
#include <deque>
#include <random>
#include <memory>
#include <Poco/Net/DatagramSocket.h>

#include "player.hpp"
#include "gameserver.hpp"

class MasterServer
{
public:
    MasterServer(uint32_t Port);
    ~MasterServer();
    
    void    init();
    void    run();
protected:
    std::mt19937                    m_oGenerator;
    std::uniform_int_distribution<> m_oDistr;
    
    std::queue<uint32_t>      m_qAvailablePorts;
    std::deque<Player>        m_aPlayersPool;
    std::vector<std::unique_ptr<GameServer>>  m_aGameServers;
    Poco::Net::DatagramSocket m_oSocket;
};

#endif /* masterserver_hpp */
