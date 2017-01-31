//
//  gameserver.hpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef gameserver_hpp
#define gameserver_hpp

#include <Poco/Net/DatagramSocket.h>
#include "player.hpp"
#include <thread>
#include <vector>

class GameServer
{
public:
    enum class State
    {
        WAITING_PLAYERS,
        RUNNING_GAME,
        FINISHED
    };
public:
    GameServer(unsigned int Port);
    ~GameServer();
    
    GameServer::State   GetState() const;
private:
    void    EventLoop();
private:
    GameServer::State   m_eState;
    std::thread         m_oThread;
    Poco::Net::DatagramSocket m_oSocket;
    
    std::vector<Player> m_aPlayers;
};

#endif /* gameserver_hpp */
