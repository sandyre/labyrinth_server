//
//  masterserver.cpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#include "masterserver.hpp"
#include <iostream>
#include <limits>
#include <memory>

#include "netpacket.hpp"
#include "msnet_generated.h"

MasterServer::MasterServer(uint32_t Port) :
m_oGenerator(228), // FIXME: random should always be random!
m_oDistr(std::uniform_int_distribution<>(std::numeric_limits<int32_t>::min(),
                                         std::numeric_limits<int32_t>::max()))
{
    Poco::Net::SocketAddress sock_addr(Poco::Net::IPAddress(), Port);
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
    for(uint32_t i = 1931; i < (1931 + 2000); ++i)
    {
        Poco::Net::DatagramSocket socket;
        Poco::Net::SocketAddress addr(Poco::Net::IPAddress(), i);
        
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
    flatbuffers::FlatBufferBuilder builder; // is needed to build output data
    Poco::Net::SocketAddress sender_addr;
    char request[256];
    
    while(true)
    {
        auto size = m_oSocket.receiveFrom(request, 256, sender_addr);
        
        auto event = MSNet::GetMSEvent(request);
        
        switch(event->event_type())
        {
            case MSNet::MSEvents_CLPing:
            {
                auto pong = MSNet::CreateMSPing(builder);
                builder.Finish(pong);
                m_oSocket.sendTo(builder.GetBufferPointer(),
                                 builder.GetSize(),
                                 sender_addr);
                builder.Clear();
                break;
            }
                
            case MSNet::MSEvents_CLFindGame:
            {
                auto finder = static_cast<const MSNet::CLFindGame*>(event->event());
                    // check that player isnt already in pool
                auto iter = std::find_if(m_aPlayersPool.begin(),
                                         m_aPlayersPool.end(),
                [finder](Player& player)
                {
                    return player.nUID == finder->player_uid();
                });
                
                    // player is not in pool already, add him
                if(iter == m_aPlayersPool.end())
                {
                    Player player;
                    player.nUID = finder->player_uid();
                    player.sock_addr = sender_addr;
                    
                    m_aPlayersPool.push_back(player);
                    
                    std::cout << "[MS] PLAYER ADDED UID: " << finder->player_uid() << "\n";
                }
                
                break;
            }
            
            default:
                std::cout << "Undefined packet received\n";
                break;
        }
        
            // if there are some players, check that there is a server for them
        if(m_aPlayersPool.size() != 0)
        {
            auto servers_available = std::count_if(m_aGameServers.begin(),
                                                   m_aGameServers.end(),
                                                   [this](std::unique_ptr<GameServer> const& gs)
                                                   {
                                                       return gs->GetState() == GameServer::State::REQUESTING_PLAYERS;
                                                   });
            
                // no servers? start a new
            if(servers_available == 0)
            {
                uint32_t nGSPort = m_qAvailablePorts.front();
                m_qAvailablePorts.pop();
                
                GameServer::Configuration config;
                config.nPlayers = 1; // +-
                config.nRandomSeed = m_oDistr(m_oGenerator);
                config.nPort = nGSPort;
                
                m_aGameServers.push_back(std::make_unique<GameServer>(config));
            }
        }
        
            // FIXME: on a big amount of gameservers, may not run correctly
        for(auto& gs : m_aGameServers)
        {
            if(m_aPlayersPool.size() == 0) // optimization (no need to loop more through gss)
                break;
            
            if(gs->GetState() == GameServer::State::REQUESTING_PLAYERS)
            {
                auto serv_config = gs->GetConfig();
                    // transfer player to GS
                auto game_found = MSNet::CreateMSGameFound(builder, serv_config.nPort);
                auto ms_event = MSNet::CreateMSEvent(builder, MSNet::MSEvents_MSGameFound, game_found.Union());
                builder.Finish(ms_event);
                
                auto& player = m_aPlayersPool.front();

                m_oSocket.sendTo(builder.GetBufferPointer(),
                                 builder.GetSize(),
                                 player.sock_addr);
                m_aPlayersPool.pop_front(); // remove a player from queue
            }
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
