//
//  masterserver.hpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef masterserver_hpp
#define masterserver_hpp

#include "GameServersController.hpp"
#include "services/system_monitor.hpp"
#include "services/DatabaseAccessor.hpp"
#include "toolkit/named_logger.hpp"

#include <Poco/Data/SessionFactory.h>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/MailMessage.h>
#include <Poco/TaskManager.h>
#include <Poco/ThreadPool.h>
#include <Poco/Timer.h>

#include <array>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <vector>


class MasterServer : public Poco::Runnable
{
private:
    class RegistrationTask;
    class LoginTask;
    class FindGameTask;

public:
    MasterServer();
    ~MasterServer();

    virtual void run() override;

protected:
    NamedLogger                             _logger;

        // Network
    Poco::Net::DatagramSocket               _socket;

        // Processing
    Poco::ThreadPool                        _taskWorkers;
    Poco::TaskManager                       _taskManager;

        // Subsystems
    std::unique_ptr<SystemMonitor>          _systemMonitor;
    std::unique_ptr<GameServersController>  _gameserversController;

    friend RegistrationTask;
    friend LoginTask;
    friend FindGameTask;
};

#endif /* masterserver_hpp */
