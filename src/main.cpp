//
//  main.cpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#include "masterserver.hpp"

int main(int argc, const char * argv[])
{
    std::unique_ptr<MasterServer> server;
    try
    {
        server = std::make_unique<MasterServer>();
    }
    catch(...)
    {
        return 1;
    }

    Poco::Thread ms_thread("MasterServer");
    ms_thread.setPriority(Poco::Thread::Priority::PRIO_HIGHEST);
    ms_thread.start(*server);
    ms_thread.join();

    return 0;
}
