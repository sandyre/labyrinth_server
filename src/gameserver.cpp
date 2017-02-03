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

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;

GameServer::GameServer(uint32_t Port) :
m_eState(GameServer::State::WAITING_PLAYERS),
m_nPort(Port),
m_nStartTime(high_resolution_clock::now()),
m_pGameWorld(nullptr)
{
    m_sServerName = "[GS";
    m_sServerName += std::to_string(m_nPort);
    m_sServerName += "]";
    
    std::cout << m_sServerName << " STARTED (WAITING PLAYERS)\n";
    
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
            using namespace GamePackets;
            CLPlayerConnect * con = reinterpret_cast<CLPlayerConnect*>(pack);
            
                // check that player doesnt exist
            auto player = FindPlayerByUID(con->nPlayerUID);
            if(player == m_aPlayers.end())
            {
                    // add player
                Player new_player;
                new_player.nUID = con->nPlayerUID;
                new_player.sock_addr = sender_addr;
                
                m_aPlayers.emplace_back(new_player);
                
                std::cout << m_sServerName << " PLAYER ATTACHED WITH UID "
                    << new_player.nUID << "\n";
            }
        }
        
        if(m_aPlayers.size() == 1)
        {
            m_eState = GameServer::State::GENERATING_WORLD;
        }
    }
    
    std::cout << m_sServerName << " GENERATING WORLD\n";
    
    if(m_eState == GameServer::State::GENERATING_WORLD)
    {
        GameWorld::Settings sets;
        sets.nSeed = 15; // FIXME: should be randomly selected
        sets.stGMSettings.nChunks = 5;
        sets.stGMSettings.nChunkWidth = 10;
        sets.stGMSettings.nChunkHeight = 10;
        m_pGameWorld = new GameWorld(sets, m_aPlayers);
        m_pGameWorld->init();
        
        std::cout << m_sServerName << " GENERATED WORLD, GAME BEGINS\n";
        
        m_eState = GameServer::State::RUNNING_GAME;
            // TODO: add waiting for players to generate levels!
    }
    
    while(m_eState == GameServer::State::RUNNING_GAME)
    {
            // m_pGameWorld->update();
        
            // send packets that GameWorld update produced
        while(!m_pGameWorld->GetEvents().empty())
        {
            GamePacket& pack = m_pGameWorld->GetEvents().front();
            SendToAll(pack);
            m_pGameWorld->GetEvents().pop();
        }
        
        m_oSocket.receiveFrom(buf, 64, sender_addr);
        
        GamePacket rcv_pack = *reinterpret_cast<GamePacket*>(buf);
        
            // update ADDR info
        auto player = FindPlayerByUID(rcv_pack.nUID);
        if(player != m_aPlayers.end())
        {
            player->sock_addr = sender_addr;
        }
        else
        {
            continue;
        }
        
        switch(rcv_pack.eType)
        {
            case GamePacket::Type::CL_MOVEMENT:
            {
                using namespace GamePackets;
                CLMovement * mov = reinterpret_cast<CLMovement*>(rcv_pack.aData);
                
                    // apply movement changes into gameworld
                auto player     = FindPlayerByUID(rcv_pack.nUID);
                player->nXCoord = mov->nXCoord;
                player->nYCoord = mov->nYCoord;
                
                    // forming answer (sending to all players)
                GamePacket ans_pack;
                SRVMovement srv_mov;
                ans_pack.eType = GamePacket::Type::SRV_MOVEMENT;
                srv_mov.nPlayerUID = rcv_pack.nUID;
                srv_mov.nXCoord = mov->nXCoord;
                srv_mov.nYCoord = mov->nYCoord;
                std::memcpy(ans_pack.aData, &srv_mov, sizeof(srv_mov));
                
                SendToAll(ans_pack);
                
                std::cout << m_sServerName << " RECEIVED MOVEMENT PACKET FROM USER "
                    << rcv_pack.nUID << " X: " << mov->nXCoord << " Y: " << mov->nYCoord << "\n";
                
                break;
            }
                
            case GamePacket::Type::CL_TAKE_EQUIP:
            {
                using namespace GamePackets;
                bool bPlayerTakesItem = false;
                CLTakeItem * take = reinterpret_cast<CLTakeItem*>(rcv_pack.aData);
                
                    // check that player can take item
                for(auto& item : m_pGameWorld->GetItems())
                {
                    if(item.nUID == take->nItemUID &&
                       item.nCarrierID == 0)
                    {
                        item.nCarrierID  = rcv_pack.nUID;
                        bPlayerTakesItem = true;
                    }
                }
                
                    // form result
                if(bPlayerTakesItem)
                {
                    GamePacket ans_pack;
                    ans_pack.eType = GamePacket::Type::SRV_TOOK_EQUIP;
                    SRVTakeItem srv_take;
                    srv_take.nItemUID = take->nItemUID;
                    srv_take.nPlayerUID = rcv_pack.nUID;
                    std::memcpy(ans_pack.aData, &srv_take, sizeof(srv_take));
                    
                    SendToAll(ans_pack);
                }
                
                std::cout << m_sServerName << " RECEIVED ITEM TAKE FROM USER " << rcv_pack.nUID
                    << " ITEM ID " << take->nItemUID << "\n";
                
                break;
            }
                
            default:
                std::cout << m_sServerName << " Undefined packet.\n";
                break;
        }
    }
    
    auto end_time = high_resolution_clock::now();
    auto duration = duration_cast<std::chrono::seconds>(end_time - m_nStartTime).count();
    std::cout << m_sServerName << " FINISHED IN " << duration << "\n";
}

void
GameServer::SendToAll(GamePacket& pack)
{
    std::for_each(m_aPlayers.cbegin(),
                  m_aPlayers.cend(),
    [&](auto& player)
    {
        m_oSocket.sendTo(&pack, sizeof(pack),
                         player.sock_addr);
    });
}

void
GameServer::SendToOne(uint32_t player_id,
                      GamePacket& pack)
{
    auto player = FindPlayerByUID(player_id);
    
    m_oSocket.sendTo(&pack, sizeof(pack),
                     player->sock_addr);
}

std::vector<Player>::iterator
GameServer::FindPlayerByUID(PlayerUID uid)
{
    for(auto iter = m_aPlayers.begin();
        iter != m_aPlayers.end();
        ++iter)
    {
        if((*iter).nUID == uid)
        {
            return iter;
        }
    }
    
    return m_aPlayers.end();
}
