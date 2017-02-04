//
//  masterserver.cpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#include "masterserver.hpp"

#include "netpacket.hpp"

MasterServer::MasterServer(uint32_t Port) :
m_nCurrentGamePort(Port+1)
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
        
        MSPacket * packet = reinterpret_cast<MSPacket*>(request);
        
        switch(packet->eType)
        {
            case MSPacket::Type::CL_PING:
            {
                packet->eType = MSPacket::Type::MS_PING;
                m_oSocket.sendTo(packet, sizeof(MSPacket),
                                 sender_addr);
                break;
            }
                
            case MSPacket::Type::CL_FIND_GAME:
            {
                m_aGameServers.push_back(new GameServer(m_nCurrentGamePort));
                
                packet->eType = MSPacket::Type::MS_GAME_FOUND;
                std::memcpy(packet->aData, &m_nCurrentGamePort, sizeof(m_nCurrentGamePort));
                m_oSocket.sendTo(packet, sizeof(MSPacket), sender_addr);
                
                ++m_nCurrentGamePort;
                break;
            }
            
            default:
                std::cout << "Undefined packet received\n";
                break;
        }
    }
}
