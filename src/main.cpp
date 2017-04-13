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
    server.init(1930);
    server.run();
    
    return 0;
}
