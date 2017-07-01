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
    MasterServer server;
    Poco::Thread ms_thread;
    ms_thread.setName("Main");
    ms_thread.setPriority(Poco::Thread::Priority::PRIO_HIGHEST);
    ms_thread.start(server);
    ms_thread.join();

    return 0;
}
