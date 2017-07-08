//
//  system_monitor.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 06.07.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef system_monitor_hpp
#define system_monitor_hpp

#include "../utils/named_logger.hpp"

#include <Poco/Timer.h>

class SystemMonitor
{
public:
    SystemMonitor();

private:
    void PrintStats(Poco::Timer& timer);

private:
    NamedLogger                     _logger;
    std::unique_ptr<Poco::Timer>    _timer;
};

#endif /* system_monitor_hpp */
