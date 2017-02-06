//
//  masterserver.cpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#include "masterserver.hpp"
#include <limits>
#include <memory>

#include "netpacket.hpp"

MasterServer::MasterServer(uint32_t Port) :
m_oGenerator(228), // FIXME: random should be always random!
m_oDistr(std::uniform_int_distribution<>(std::numeric_limits<int32_t>::min(),
                                         std::numeric_limits<int32_t>::max()))
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
MasterServer::init()
{
    for(uint32_t i = 1931; i < 1931 + 2000; ++i)
    {
        Poco::Net::DatagramSocket socket;
        Poco::Net::SocketAddress addr("localhost", i);
        
        try
        {
            socket.bind(addr);
            m_qAvailablePorts.push(i);
            socket.close();
        } catch (std::exception e)
        {
                // port is busy
        }
    }
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
                    // check that player isnt already in pool
                auto iter = std::find_if(m_aPlayersPool.begin(),
                                         m_aPlayersPool.end(),
                [&](Player& player)
                {
                    return player.nUID == packet->nUID;
                });
                
                    // player is not in pool already, add him
                if(iter == m_aPlayersPool.end())
                {
                    Player player;
                    player.nUID = packet->nUID;
                    player.sock_addr = sender_addr;
                    
                    m_aPlayersPool.push_back(player);
                    
                    std::cout << "[MS] PLAYER ADDED UID: " << packet->nUID << "\n";
                }
                
                break;
            }
            
            default:
                std::cout << "Undefined packet received\n";
                break;
        }
        
            // create lobby for ready players
        if(m_aPlayersPool.size() == 2)
        {
            uint32_t nGSPort = m_qAvailablePorts.front();
            m_qAvailablePorts.pop();
            
            GameServer::Configuration config;
            config.nPlayers = 2;
            config.nRandomSeed = m_oDistr(m_oGenerator);
            config.nPort = nGSPort;
            
            m_aGameServers.push_back(std::make_unique<GameServer>(config));
            
            MSPacket pack;
            pack.eType = MSPacket::Type::MS_GAME_FOUND;
            std::memcpy(pack.aData, &nGSPort, sizeof(nGSPort));
            for(auto& player : m_aPlayersPool)
            {
                m_oSocket.sendTo(&pack, sizeof(MSPacket), player.sock_addr);
            }
            m_aPlayersPool.clear();
        }
        
            // do something with finished servers
        m_aGameServers.erase(
                             std::remove_if(m_aGameServers.begin(),
                                            m_aGameServers.end(),
                                            [this](std::unique_ptr<GameServer> const& server)
                                            {
                                                m_qAvailablePorts.push(server->GetConfig().nPort);
                                                return server->GetState() == GameServer::State::FINISHED;
                                            }),
                             m_aGameServers.end()
                             );
    }
}
