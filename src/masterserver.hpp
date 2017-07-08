//
//  masterserver.hpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef masterserver_hpp
#define masterserver_hpp

#include "flatbuffers/flatbuffers.h"

#include "gameserver.hpp"
#include "player.hpp"
#include "services/system_monitor.hpp"
#include "utils/named_logger.hpp"

#include <Poco/Data/SessionFactory.h>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/MailMessage.h>
#include <Poco/Timer.h>
#include <Poco/ThreadPool.h>

#include <array>
#include <deque>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

class MasterServer : public Poco::Runnable
{
    using GameServers = std::vector<std::unique_ptr<GameServer>>;

public:
    struct SystemStatus
    {
        static const int LOG_SYSTEM_ACTIVE      = 0x01;
        static const int NETWORK_SYSTEM_ACTIVE  = 0x02;
        static const int EMAIL_SYSTEM_ACTIVE    = 0x04;
        static const int DATABASE_SYSTEM_ACTIVE = 0x08;
    };

public:
    MasterServer();
    ~MasterServer();

    void init(uint32_t Port);

    virtual void run();

protected:
    void service_loop();
    
    void FreeResourcesAndSaveResults(Poco::Timer&);

protected:
    std::mt19937                    _randGenerator;
    std::uniform_int_distribution<> _randDistr;

    std::queue<uint32_t>            _availablePorts;

    uint32_t m_nSystemStatus;

    // Gameservers
    std::unique_ptr<Poco::ThreadPool>        _threadPool;
    std::mutex                               _serversMutex;
    GameServers                              _gameServers;

    // System timers and flags
    std::unique_ptr<Poco::Timer>   _freementTimer;

    // Network
    Poco::Net::DatagramSocket      _socket;
    flatbuffers::FlatBufferBuilder _flatBuilder;
    std::array<uint8_t, 512>       _dataBuffer;

    // Logging
    NamedLogger         _logger;

    // Labyrinth database
    std::unique_ptr<Poco::Data::Session> _dbSession;

        // Services
    std::unique_ptr<SystemMonitor>       _systemMonitor;
};

#endif /* masterserver_hpp */
