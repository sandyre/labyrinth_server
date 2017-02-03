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
#include "gameworld.hpp"
#include <thread>
#include <vector>
#include <chrono>
#include <string>

using std::chrono::high_resolution_clock;

class GameServer
{
public:
    enum class State
    {
        WAITING_PLAYERS,
        GENERATING_WORLD,
        RUNNING_GAME,
        FINISHED
    };
public:
    GameServer(uint32_t Port);
    ~GameServer();
    
    GameServer::State   GetState() const;
private:
    void    EventLoop();
    void    SendToOne(uint32_t, GamePacket&);
    void    SendToAll(GamePacket&);
    
    inline std::vector<Player>::iterator FindPlayerByUID(PlayerUID);
private:
    GameServer::State   m_eState;
    std::string         m_sServerName;
    std::thread         m_oThread;
    Poco::Net::DatagramSocket m_oSocket;
    int32_t m_nPort;
    high_resolution_clock::time_point m_nStartTime;
    
    GameWorld * m_pGameWorld;
    std::vector<Player> m_aPlayers;
};

#endif /* gameserver_hpp */
