//
//  masterserver.cpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#include "masterserver.hpp"

#include "packet.hpp"

MasterServer::MasterServer(unsigned int Port)
{
    Poco::Net::SocketAddress sock_addr("localhost", Port);
    m_oSocket.bind(sock_addr);
    
    std::cout << "MasterServer started at port " << Port << "\n";
    std::cout << "Available CPU cores: " << std::thread::hardware_concurrency() << "\n";
}

MasterServer::~MasterServer()
{
    m_oSocket.close();
    std::cout << "MasterServer shut down.\n";
}

void
MasterServer::run()
{
    Poco::Net::SocketAddress sender_addr;
    char request[64];
    
    while(true)
    {
        m_oSocket.receiveFrom(request, 64, sender_addr);
        
        Packet packet = *reinterpret_cast<Packet*>(request);
        
        std::cout << "Received packet from " << sender_addr.host().toString() << "\n";
        
        switch(packet.eType)
        {
            case Packet::Type::PING:
            {
                m_oSocket.sendTo("pong", 4, sender_addr);
                break;
            }
                
            case Packet::Type::SEARCH_GAME:
            {
                Player player;
                player.x = player.y = 0;
                player.uid = 0;
                player.sock_addr = sender_addr;
                m_aPlayersPool.push_back(player);
                break;
            }
            
            default:
                std::cout << "Undefined packet received\n";
                break;
        }
    }
}
