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
#include <memory>
#include "gsnet_generated.h"

using namespace GameEvent;
using namespace std::chrono;

GameServer::GameServer(const Configuration& config) :
m_eState(GameServer::State::REQUESTING_PLAYERS),
m_stConfig(config),
m_nStartTime(steady_clock::now()),
m_msPerUpdate(0)
{
    m_sServerName = "GS";
    m_sServerName += std::to_string(m_stConfig.nPort);
    
    m_oLogSys.Init(m_sServerName, LogSystem::Mode::STDIO);
    
    m_oMsgBuilder << "Started. Configuration: {SEED:" << m_stConfig.nRandomSeed;
    m_oMsgBuilder << "; PLAYERS_COUNT: " << m_stConfig.nPlayers << "}";
    m_oLogSys.Write(m_oMsgBuilder.str());
    m_oMsgBuilder.str("");
    
    Poco::Net::SocketAddress addr(Poco::Net::IPAddress(), m_stConfig.nPort);
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
    m_oLogSys.Close();
}

GameServer::State
GameServer::GetState() const
{
    return m_eState;
}

GameServer::Configuration
GameServer::GetConfig() const
{
    return m_stConfig;
}

void
GameServer::EventLoop()
{
    flatbuffers::FlatBufferBuilder builder;
    
    Poco::Net::SocketAddress sender_addr;
    unsigned char buf[256];
    
    while(m_eState == GameServer::State::REQUESTING_PLAYERS)
    {
        auto size = m_oSocket.receiveFrom(buf, 256, sender_addr);
        
        auto gs_event = GetEvent(buf);
        
        if(gs_event->event_type() == Events_CLConnection)
        {
            auto con_info = static_cast<const CLConnection*>(gs_event->event());
            
            auto player = FindPlayerByUID(con_info->player_uid());
            if(player == m_aPlayers.end())
            {
                Player new_player;
                new_player.nUID = con_info->player_uid();
                new_player.sock_addr = sender_addr;
                strncpy(new_player.sNickname,
                        con_info->nickname()->c_str(),
                        con_info->nickname()->size());
                
                m_aPlayers.emplace_back(new_player);
                
                m_oMsgBuilder << "Player \'" << new_player.sNickname << "\' connected";
                m_oLogSys.Write(m_oMsgBuilder.str());
                m_oMsgBuilder.str("");
                
                    // notify connector that he is accepted
                auto gs_accept = CreateSVConnectionStatus(builder,
                                                          ConnectionStatus_ACCEPTED);
                auto gs_event = CreateEvent(builder,
                                            Events_SVConnectionStatus,
                                            gs_accept.Union());
                builder.Finish(gs_event);
                
                m_oSocket.sendTo(builder.GetBufferPointer(),
                                 builder.GetSize(),
                                 sender_addr);
                builder.Clear();
                
                    // send him info about all current players in lobby
                for(auto& player : m_aPlayers)
                {
                    auto nick = builder.CreateString(player.sNickname);
                    auto player_info = CreateSVPlayerConnected(builder,
                                                               player.nUID,
                                                               nick);
                    gs_event = CreateEvent(builder,
                                           Events_SVPlayerConnected,
                                           player_info.Union());
                    builder.Finish(gs_event);
                    
                    m_oSocket.sendTo(builder.GetBufferPointer(),
                                     builder.GetSize(),
                                     sender_addr);
                    builder.Clear();
                }
                
                    // notify all players about connected one
                auto nick = builder.CreateString(con_info->nickname()->c_str(),
                                                 con_info->nickname()->size());
                auto gs_new_player = CreateSVPlayerConnected(builder,
                                                             con_info->player_uid(),
                                                             nick);
                gs_event = CreateEvent(builder,
                                       Events_SVPlayerConnected,
                                       gs_new_player.Union());
                builder.Finish(gs_event);
                
                SendToAll(builder.GetBufferPointer(),
                          builder.GetSize());
                builder.Clear();
            }
        }
        
        if(m_aPlayers.size() == m_stConfig.nPlayers)
        {
            m_eState = GameServer::State::GENERATING_WORLD;
        }
    }
    
    m_oMsgBuilder << "Generating world";
    m_oLogSys.Write(m_oMsgBuilder.str());
    m_oMsgBuilder.str("");
    
    if(m_eState == GameServer::State::GENERATING_WORLD)
    {
        GameWorld::Settings sets;
        sets.nSeed = m_stConfig.nRandomSeed;
        sets.stGMSettings.nSeed = m_stConfig.nRandomSeed;
        sets.stGMSettings.nMapSize = 3;
        sets.stGMSettings.nRoomSize = 10;
        m_pGameWorld = std::make_unique<GameWorld>(sets, m_aPlayers);
        m_pGameWorld->generate_map();
        m_pGameWorld->initial_spawn(); // spawns players
        
        auto gs_gen_map = CreateSVGenerateMap(builder,
                                              sets.stGMSettings.nMapSize,
                                              sets.stGMSettings.nRoomSize,
                                              sets.nSeed);
        auto gs_event = CreateEvent(builder,
                                    Events_SVGenerateMap,
                                    gs_gen_map.Union());
        builder.Finish(gs_event);
        
        SendToAll(builder.GetBufferPointer(), builder.GetSize());
        builder.Clear();
        
        auto players_ungenerated = m_aPlayers.size();
        while(players_ungenerated)
        {
            m_oSocket.receiveBytes(buf, 256);
            
            auto gs_event = GetEvent(buf);
            
            if(gs_event->event_type() == Events_CLMapGenerated)
            {
                auto cl_gen_ok = static_cast<const CLMapGenerated*>(gs_event->event());
                
                m_oMsgBuilder << "Player " << cl_gen_ok->player_uid();
                m_oMsgBuilder << " generated map";
                m_oLogSys.Write(m_oMsgBuilder.str());
                m_oMsgBuilder.str("");
                
                --players_ungenerated;
            }
        }
        
        m_eState = GameServer::State::RUNNING_GAME;
    }
    
        // notify players that game starts!
    {
        auto game_start = CreateSVGameStart(builder);
        auto gs_event = CreateEvent(builder,
                                    Events_SVGameStart,
                                    game_start.Union());
        builder.Finish(gs_event);
        SendToAll(builder.GetBufferPointer(), builder.GetSize());
        builder.Clear();
        
        m_oMsgBuilder << "Game begins";
        m_oLogSys.Write(m_oMsgBuilder.str());
        m_oMsgBuilder.str("");
    }
    
    int times_skipped = 0;
    while(m_eState == GameServer::State::RUNNING_GAME)
    {
        auto frame_start = steady_clock::now();
        
        bool event_received = false;
        bool is_event_valid = false;
        size_t bytes_read = 0;
        
        auto& out_events = m_pGameWorld->GetOutEvents();
        while(out_events.size())
        {
            auto event = out_events.front();
            SendToAll(event.data(), event.size());
            out_events.pop();
        }
        
            // if game ended
        if(m_pGameWorld->GetState() == GameWorld::State::FINISHED)
        {
            m_eState = GameServer::State::FINISHED;
        }
        
            // if event received, process it
        if(m_oSocket.available())
        {
            bytes_read = m_oSocket.receiveFrom(buf, 256, sender_addr);
            event_received = true;
            times_skipped = 0;
        }
        else // sleep and update gameworld overwise (50 updates ps)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            event_received = false;
            times_skipped++;
        }
        
        if(event_received)
        {
            auto gs_event = GetEvent(buf);
            
            switch(gs_event->event_type())
            {
                case Events_CLActionMove:
                {
                    auto cl_mov = static_cast<const CLActionMove*>(gs_event->event());
                    auto player = FindPlayerByUID(cl_mov->target_uid());
                    
                    if(player != m_aPlayers.end() &&
                       player->sock_addr != sender_addr)
                    {
                        player->sock_addr = sender_addr;
                    }
                    
                    is_event_valid = true;
                    break;
                }
                    
                case Events_CLActionItem:
                {
                    auto cl_item = static_cast<const CLActionItem*>(gs_event->event());
                    auto player = FindPlayerByUID(cl_item->player_uid());
                    
                    if(player != m_aPlayers.end() &&
                       player->sock_addr != sender_addr)
                    {
                        player->sock_addr = sender_addr;
                    }
                    
                    is_event_valid = true;
                    break;
                }
                    
                case Events_CLActionSwamp:
                {
                    auto cl_swamp = static_cast<const CLActionSwamp*>(gs_event->event());
                    auto player = FindPlayerByUID(cl_swamp->player_uid());
                    
                    if(player != m_aPlayers.end() &&
                       player->sock_addr != sender_addr)
                    {
                        player->sock_addr = sender_addr;
                    }
                    
                    is_event_valid = true;
                    break;
                }
                    
                case Events_CLActionDuel:
                {
                    auto cl_duel = static_cast<const CLActionDuel*>(gs_event->event());
                    
                        //FIXME: players validation needed?
                    
                    is_event_valid = true;
                    break;
                }
                    
                default:
                    assert(false);
                    break;
            }
            
            if(is_event_valid) // add received event to the gameworld
            {
                auto& in_events = m_pGameWorld->GetInEvents();
                std::vector<uint8_t> event(buf,
                                           buf + bytes_read);
                in_events.emplace(event);
            }
        }
        
        auto frame_end = steady_clock::now();
        
        m_pGameWorld->update(duration_cast<milliseconds>(frame_end-frame_start));
        
        if(times_skipped > 20000)
        {
            m_eState = GameServer::State::FINISHED;
        }
    }
    
    auto end_time = steady_clock::now();
    auto duration = duration_cast<seconds>(end_time - m_nStartTime).count();
    m_oMsgBuilder << "Finished in " << duration << " seconds";
    m_oLogSys.Write(m_oMsgBuilder.str());
    m_oMsgBuilder.str("");
}

void
GameServer::SendToAll(uint8_t * buf,
                      size_t size)
{
    std::for_each(m_aPlayers.cbegin(),
                  m_aPlayers.cend(),
                  [buf, size, this](const Player& player)
                  {
                      m_oSocket.sendTo(buf, size,
                                       player.sock_addr);
                  });
}

void
GameServer::SendToOne(uint32_t player_id,
                      uint8_t * buf,
                      size_t size)
{
    auto player = FindPlayerByUID(player_id);
    
    if(player != m_aPlayers.end())
    {
        m_oSocket.sendTo(buf,
                         size,
                         player->sock_addr);
    }
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
