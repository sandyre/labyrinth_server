//
//  masterserver.hpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef masterserver_hpp
#define masterserver_hpp

#include <array>
#include <thread>
#include <vector>
#include <deque>
#include <random>
#include <memory>
#include <mutex>
#include <sstream>
#include <Poco/ThreadPool.h>
#include <Poco/Timer.h>
#include <Poco/Net/MailMessage.h>
#include <Poco/Net/SMTPClientSession.h>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Data/SessionFactory.h>
#include "flatbuffers/flatbuffers.h"


#include "shell.hpp"
#include "player.hpp"
#include "gameserver.hpp"
#include "logsystem.hpp"
#include "mailservice.hpp"

class MasterServer
{
public:
    struct SystemStatus
    {
        static const int LOG_SYSTEM_ACTIVE = 0x01;
        static const int NETWORK_SYSTEM_ACTIVE = 0x02;
        static const int EMAIL_SYSTEM_ACTIVE = 0x04;
        static const int DATABASE_SYSTEM_ACTIVE = 0x08;
    };
public:
    MasterServer();
    ~MasterServer();
    
    void    init(uint32_t Port);
    
    virtual void run();
    
protected:
    void    ProcessIncomingMessage();
    void    CommutatePlayers();
    void    FreeResourcesAndSaveResults(Poco::Timer&);
    
    std::mt19937                    m_oGenerator;
    std::uniform_int_distribution<> m_oDistr;
    
    std::queue<uint32_t>      m_qAvailablePorts;
    std::deque<Player>        m_aPlayersPool;
    
    uint32_t    m_nSystemStatus;
    
        // Gameservers
    std::unique_ptr<Poco::ThreadPool>    m_oThreadPool;
    std::mutex m_oGSMutex;
    std::vector<std::unique_ptr<GameServer>>  m_aGameServers;
    
        // System timers and flags
    std::unique_ptr<Poco::Timer>    m_poFreementTimer;
    bool        m_bCommutationNeeded;
    
        // MailService and timer for it
    MailService m_oMailService;
    std::unique_ptr<Poco::Timer>    m_poMailTimer;
    
        // Network
    Poco::Net::DatagramSocket m_oSocket;
    flatbuffers::FlatBufferBuilder m_oFlatBuilder;
    std::array<uint8_t, 512> m_aDataBuffer;
    
        // Logging
    LogSystem m_oLogSys;
    std::ostringstream m_oMsgBuilder;
    
        // E-MAIL
    std::unique_ptr<Poco::Net::SMTPClientSession> m_poMailClient;
    
        // Labyrinth database
    std::unique_ptr<Poco::Data::Session> m_poDBSession;
    
        // Shell
    std::unique_ptr<Shell> m_poShell;
    
    friend Shell;
};

#endif /* masterserver_hpp */
