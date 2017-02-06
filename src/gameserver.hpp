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
#include <cstring>

using std::chrono::high_resolution_clock;
using std::chrono::steady_clock;

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
    
    struct Configuration
    {
        uint32_t nPort;
        uint32_t nRandomSeed;
        uint16_t nPlayers;
    };
public:
    GameServer(const Configuration&);
    ~GameServer();
    
    GameServer::State   GetState() const;
    GameServer::Configuration GetConfig() const;
private:
    void    EventLoop();
    void    SendToOne(uint32_t, GamePacket&);
    void    SendToAll(GamePacket&);
    
    inline std::vector<Player>::iterator FindPlayerByUID(PlayerUID);
private:
    GameServer::State   m_eState;
    GameServer::Configuration m_stConfig;
    std::string         m_sServerName;
    std::thread         m_oThread;
    Poco::Net::DatagramSocket m_oSocket;
    high_resolution_clock::time_point m_nStartTime;
    std::chrono::milliseconds         m_msPerUpdate;
    
    std::unique_ptr<GameWorld>  m_pGameWorld;
    std::vector<Player> m_aPlayers;
};

#endif /* gameserver_hpp */
