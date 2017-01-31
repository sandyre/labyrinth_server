//
//  gameserver.cpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#include "gameserver.hpp"

#include <iostream>
#include "packet.hpp"
#include <Poco/Checksum.h>

GameServer::GameServer(unsigned int Port) :
m_eState(GameServer::State::WAITING_PLAYERS)
{
    std::cout << "[GS" << Port << "] STARTED\n";
    
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
    
    while(m_eState != GameServer::State::FINISHED)
    {
        m_oSocket.receiveFrom(buf, 64, sender_addr);
        
        GamePacket * pack = reinterpret_cast<GamePacket*>(buf);
        
            // check that packet was not damaged
        Poco::Checksum crc;
        crc.update(buf, sizeof(GamePacket) - 4);
        
        if(pack->nCRC32 != crc.checksum())
        {
                // TODO: add external logic if packet was damaged
            continue;
        }
        
            // update ADDR info
        for(auto& player : m_aPlayers)
        {
            if(player.uid == pack->nPlayerUID)
            {
                player.sock_addr = sender_addr;
            }
        }
        
        switch(pack->eType)
        {
            case GamePacket::Type::PING:
            {
                Poco::Checksum crc;
                GamePacket packet;
                packet.eType = GamePacket::Type::PING;
                packet.nPlayerUID = 0; // 0 = SERVER
                std::memset(packet.aContent, 0, 16); // may be excessive
                crc.update((char*)&packet, sizeof(packet)-4);
                packet.nCRC32 = crc.checksum();
                
                m_oSocket.sendTo((char*)&packet, sizeof(packet), sender_addr);
                break;
            }
                
            default:
                std::cout << "Undefined packet.\n";
                break;
        }
    }
}
