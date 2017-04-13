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
#include <Poco/Runnable.h>
#include "player.hpp"
#include "gameworld.hpp"
#include "logsystem.hpp"
#include <thread>
#include <vector>
#include <chrono>
#include <string>
#include <cstring>
#include <sstream>
#include <memory>

using std::chrono::steady_clock;

class GameServer : public Poco::Runnable
{
public:
    enum class State
    {
        LOBBY_FORMING,
        HERO_PICK,
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
    
    virtual void run();
    
    GameServer::State   GetState() const;
    GameServer::Configuration GetConfig() const;
private:
    void    shutdown();
    
    void    SendToOne(uint32_t, uint8_t *, size_t);
    void    SendToAll(uint8_t *, size_t);
    
    inline std::vector<Player>::iterator FindPlayerByUID(PlayerUID);
private:
    GameServer::State   m_eState;
    GameServer::Configuration m_stConfig;
    std::string         m_sServerName;
    Poco::Net::DatagramSocket m_oSocket;
    steady_clock::time_point          m_nStartTime;
    std::chrono::milliseconds         m_msPerUpdate;
    
    std::unique_ptr<GameWorld>  m_pGameWorld;
    std::vector<Player> m_aPlayers;
    
    LogSystem m_oLogSys;
    std::ostringstream m_oMsgBuilder;
    
    flatbuffers::FlatBufferBuilder  m_oBuilder;
};

#endif /* gameserver_hpp */
