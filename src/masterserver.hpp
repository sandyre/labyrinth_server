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
#include <sstream>
#include <Poco/Net/MailMessage.h>
#include <Poco/Net/SMTPClientSession.h>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Data/Session.h>

#include "player.hpp"
#include "gameserver.hpp"
#include "logsystem.hpp"

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
    void    run();
protected:
    std::mt19937                    m_oGenerator;
    std::uniform_int_distribution<> m_oDistr;
    
    std::queue<uint32_t>      m_qAvailablePorts;
    std::deque<Player>        m_aPlayersPool;
    std::vector<std::unique_ptr<GameServer>>  m_aGameServers;
    Poco::Net::DatagramSocket m_oSocket;
    
    uint32_t    m_nSystemStatus;
    
        // Logging
    LogSystem m_oLogSys;
    std::ostringstream m_oMsgBuilder;
    
        // E-MAIL
    Poco::Net::SMTPClientSession m_oMailClient;
    
        // Players database
    std::unique_ptr<Poco::Data::Session> m_pDBSession;
};

#endif /* masterserver_hpp */
