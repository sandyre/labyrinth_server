//
//  gameserver.cpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#include "gameserver.hpp"

#include <algorithm>
#include <iostream>
#include "netpacket.hpp"
#include <Poco/Checksum.h>

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;

GameServer::GameServer(uint32_t Port) :
m_eState(GameServer::State::WAITING_PLAYERS),
m_nPort(Port),
m_nStartTime(high_resolution_clock::now())
{
    std::cout << "[GS" << Port << "] STARTED (WAITING PLAYERS)\n";
    
    Poco::Net::SocketAddress addr("localhost", Port);
    m_oSocket.bind(addr);
    
    m_oThread = std::thread(
    [this]()
    {
        this->EventLoop();
    });
    m_oThread.detach();
}

GameServer::~GameServer()
{
    m_oSocket.close();
}

GameServer::State
GameServer::GetState() const
{
    return m_eState;
}

void
GameServer::EventLoop()
{
    Poco::Net::SocketAddress sender_addr;
    char buf[64];
    
    while(m_eState == GameServer::State::WAITING_PLAYERS)
    {
        m_oSocket.receiveFrom(buf, 64, sender_addr);
        
        GamePacket * pack = reinterpret_cast<GamePacket*>(buf);
        if(pack->eType == GamePacket::Type::CL_CONNECT)
        {
            auto iter =
            std::find_if(m_aPlayers.begin(), m_aPlayers.end(),
            [=](Player& player)
            {
                return player.sock_addr == sender_addr;
            });
            
            if(iter != m_aPlayers.end() || m_aPlayers.size() == 0)
            {
                Player player;
                player.nUID = 0;
                player.nXCoord = 0;
                player.nYCoord = 0;
                player.sock_addr = sender_addr;
                
                m_aPlayers.push_back(player);
                
                GamePackets::SetPlayerUID set_uid;
                set_uid.nUID = player.nUID;
                
                pack->eType = GamePacket::Type::SRV_PL_SET_UID;
                std::memcpy(pack->aData, &set_uid, sizeof(set_uid));
                
                m_oSocket.sendTo(pack, sizeof(GamePacket),
                                 sender_addr);
                
                std::cout << "[GS" << m_nPort << "] PLAYER ATTACHED WITH UID " << player.nUID << "\n";
            }
        }
        
        if(m_aPlayers.size() == 1)
        {
            m_eState = GameServer::State::GENERATING_WORLD;
        }
    }
    
    std::cout << "[GS" << m_nPort << "] GENERATING WORLD\n";
    
    while(m_eState == GameServer::State::GENERATING_WORLD)
    {
        break;
    }
    
    while(m_eState != GameServer::State::FINISHED)
    {
        m_oSocket.receiveFrom(buf, 64, sender_addr);
        
        GamePacket * pack = reinterpret_cast<GamePacket*>(buf);
        
            // update ADDR info
        for(auto& player : m_aPlayers)
        {
            if(player.nUID == pack->nUID)
            {
                player.sock_addr = sender_addr;
            }
        }
        
        switch(pack->eType)
        {
            case GamePacket::Type::CL_MOVEMENT:
            {
                using namespace GamePackets;
                Movement * mov = reinterpret_cast<Movement*>(pack->aData);
                
                for(auto& player : m_aPlayers)
                {
                    if(player.nUID == pack->nUID)
                    {
                        player.nXCoord = mov->nXCoord;
                        player.nYCoord = mov->nYCoord;
                        
                        break;
                    }
                }
                
                pack->eType = GamePacket::Type::SRV_MOVEMENT;
                
                for(auto& player : m_aPlayers)
                {
                    m_oSocket.sendTo(pack,
                                     sizeof(GamePacket),
                                     player.sock_addr);
                }
                
                std::cout << "[GS" << m_nPort << "] RECEIVED MOVEMENT PACKET FROM USER "
                    << pack->nUID << " X: " << mov->nXCoord << " Y: " << mov->nYCoord << "\n";
                
                break;
            }
                
            default:
                std::cout << "[GS" << m_nPort << "] Undefined packet.\n";
                break;
        }
    }
    
    auto end_time = high_resolution_clock::now();
    auto duration = duration_cast<std::chrono::seconds>(end_time - m_nStartTime).count();
    std::cout << "[GS" << m_nPort << "] FINISHED IN " << duration << "\n";
}
