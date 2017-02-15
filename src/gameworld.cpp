    //
    //  gameworld.cpp
    //  labyrinth_server
    //
    //  Created by Aleksandr Borzikh on 02.02.17.
    //  Copyright © 2017 sandyre. All rights reserved.
    //

#include "gameworld.hpp"

#include "gsnet_generated.h"
#include <random>

using namespace GameEvent;

GameWorld::GameWorld(GameWorld::Settings& sets,
                     std::vector<Player>& players) :
m_stSettings(sets),
m_aPlayers(players)
{
}

void
GameWorld::generate_map()
{
    m_oGameMap = GameMap(m_stSettings.stGMSettings);
}

void
GameWorld::initial_spawn()
{
    std::vector<uint8_t> data;
        // spawn players
    for(auto& player : m_aPlayers)
    {
        Point2 spawn_point = GetRandomPosition();
        player.stPosition.x = spawn_point.x;
        player.stPosition.y = spawn_point.y;
        
        auto gs_spawn = CreateSVSpawnPlayer(m_oBuilder,
                                            player.nUID,
                                            player.stPosition.x,
                                            player.stPosition.y);
        auto gs_event = CreateEvent(m_oBuilder,
                                    Events_SVSpawnPlayer,
                                    gs_spawn.Union());
        m_oBuilder.Finish(gs_event);
        
        data.assign(m_oBuilder.GetBufferPointer(),
                    m_oBuilder.GetBufferPointer() + m_oBuilder.GetSize());
        
        m_aOutEvents.emplace(data);
        m_oBuilder.Clear();
    }
    
        // make a key
    auto pos = GetRandomPosition();
    Item item;
    item.eType = Item::Type::KEY;
    item.nUID = 0;
    item.stPosition.x = pos.x;
    item.stPosition.y = pos.y;
    m_aItems.push_back(item);
    
    auto gs_item = CreateSVSpawnItem(m_oBuilder,
                                     item.nUID,
                                     ItemType_KEY,
                                     item.stPosition.x,
                                     item.stPosition.y);
    auto gs_event = CreateEvent(m_oBuilder,
                                Events_SVSpawnItem,
                                gs_item.Union());
    m_oBuilder.Finish(gs_event);
    
    data.assign(m_oBuilder.GetBufferPointer(),
                m_oBuilder.GetBufferPointer() + m_oBuilder.GetSize());
    m_aOutEvents.emplace(data);
    m_oBuilder.Clear();
    
        // make swamp
    pos = GetRandomPosition();
    Construction constr;
    constr.eType = Construction::Type::SWAMP;
    constr.nUID = 0;
    constr.stPosition.x = pos.x;
    constr.stPosition.y = pos.y;
    m_aConstructions.push_back(constr);
    
    auto gs_constr = CreateSVSpawnConstr(m_oBuilder,
                                         constr.nUID,
                                         ConstrType_SWAMP,
                                         constr.stPosition.x,
                                         constr.stPosition.y);
    gs_event = CreateEvent(m_oBuilder,
                           Events_SVSpawnConstr,
                           gs_constr.Union());
    m_oBuilder.Finish(gs_event);
    
    data.assign(m_oBuilder.GetBufferPointer(),
                m_oBuilder.GetBufferPointer() + m_oBuilder.GetSize());
    m_aOutEvents.emplace(data);
    m_oBuilder.Clear();
}

void
GameWorld::update(std::chrono::milliseconds ms)
{
    while(!m_aInEvents.empty()) // received packets are already validated
    {
        auto event = m_aInEvents.front();
        auto gs_event = GetEvent(event.data());
        
        switch(gs_event->event_type())
        {
            case Events_CLActionMove:
            {
                auto cl_mov = static_cast<const CLActionMove*>(gs_event->event());
                auto player = GetPlayerByUID(cl_mov->player_uid());
                
                    // check that player CAN walk
                if(player->eState != Player::State::WALKING)
                    break;
                
                    // apply movement changes
                player->stPosition.x = cl_mov->x();
                player->stPosition.y = cl_mov->y();
                
                auto gs_mov = CreateSVActionMove(m_oBuilder,
                                                 player->nUID,
                                                 player->stPosition.x,
                                                 player->stPosition.y);
                auto gs_ev = CreateEvent(m_oBuilder,
                                         Events_SVActionMove,
                                         gs_mov.Union());
                m_oBuilder.Finish(gs_ev);
                
                std::vector<uint8_t> packet(m_oBuilder.GetBufferPointer(),
                                            m_oBuilder.GetBufferPointer() +
                                            m_oBuilder.GetSize());
                m_aOutEvents.emplace(packet);
                
                m_oBuilder.Clear();
                break;
            }
                
            case Events_CLActionSwamp:
            {
                auto cl_swamp = static_cast<const CLActionSwamp*>(gs_event->event());
                auto player = GetPlayerByUID(cl_swamp->player_uid());
                
                if(cl_swamp->status() == GameEvent::ActionSwampStatus_ESCAPED)
                {
                    player->eState = Player::State::WALKING;
                    
                        // move player away from swamp
                    auto& map = m_oGameMap.GetMap();
                    if(map[player->stPosition.x-1][player->stPosition.y] ==
                       GameMap::MapBlockType::NOBLOCK)
                    {
                        player->stPosition.x--;
                    }
                    else if(map[player->stPosition.x+1][player->stPosition.y] ==
                            GameMap::MapBlockType::NOBLOCK)
                    {
                        player->stPosition.x++;
                    }
                    else if(map[player->stPosition.x][player->stPosition.y-1] ==
                            GameMap::MapBlockType::NOBLOCK)
                    {
                        player->stPosition.y--;
                    }
                    else if(map[player->stPosition.x+1][player->stPosition.y+1] ==
                            GameMap::MapBlockType::NOBLOCK)
                    {
                        player->stPosition.y++;
                    }
                    
                    auto sv_move = CreateSVActionMove(m_oBuilder,
                                                      player->nUID,
                                                      player->stPosition.x,
                                                      player->stPosition.y);
                    auto sv_event = CreateEvent(m_oBuilder,
                                                Events_SVActionMove,
                                                sv_move.Union());
                    m_oBuilder.Finish(sv_event);
                    
                    std::vector<uint8_t> packet(m_oBuilder.GetBufferPointer(),
                                                m_oBuilder.GetBufferPointer() +
                                                m_oBuilder.GetSize());
                    m_aOutEvents.emplace(packet);
                    
                    m_oBuilder.Clear();
                }
            }
                
            default:
                assert(false);
                break;
        }
        
        m_aInEvents.pop(); // remove packet
    }
    
        // timer for players in swamp
    for(auto& player : m_aPlayers)
    {
        if(player.eState == Player::State::SWAMP)
        {
            player.nTimer += ms.count();
            
            if(player.nTimer > 5000)
            {
                player.nTimer = 0;
                player.eState = Player::State::DEAD;
                
                auto gs_swamp = CreateSVActionSwamp(m_oBuilder,
                                                    player.nUID,
                                                    ActionSwampStatus_DIED);
                auto gs_event = CreateEvent(m_oBuilder,
                                            Events_SVActionSwamp,
                                            gs_swamp.Union());
                m_oBuilder.Finish(gs_event);
                
                std::vector<uint8_t> packet(m_oBuilder.GetBufferPointer(),
                                            m_oBuilder.GetBufferPointer() +
                                            m_oBuilder.GetSize());
                m_aOutEvents.emplace(packet);
                
                m_oBuilder.Clear();
            }
        }
    }
    
        // check that player isnt in swamp
    for(auto& player : m_aPlayers)
    {
        for(auto& constr : m_aConstructions)
        {
            if(constr.eType == Construction::Type::SWAMP &&
               player.eState != Player::State::SWAMP &&
               player.eState != Player::State::DEAD && // player should not get in twice
               player.stPosition == constr.stPosition)
            {
                player.eState = Player::State::SWAMP;
                
                auto sv_swamp = CreateSVActionSwamp(m_oBuilder,
                                                    player.nUID,
                                                    ActionSwampStatus_STARTED);
                auto sv_event = CreateEvent(m_oBuilder,
                                            Events_SVActionSwamp,
                                            sv_swamp.Union());
                m_oBuilder.Finish(sv_event);
                
                std::vector<uint8_t> packet(m_oBuilder.GetBufferPointer(),
                                            m_oBuilder.GetBufferPointer() +
                                            m_oBuilder.GetSize());
                m_aOutEvents.emplace(packet);
                
                m_oBuilder.Clear();
                break;
            }
        }
    }
    
        // respawn dead players
    for(auto& player : m_aPlayers)
    {
        if(player.eState == Player::State::DEAD)
        {
            player.nTimer += ms.count();
            
            if(player.nTimer > 3000)
            {
                player.nTimer = 0;
                
                player.eState = Player::State::WALKING;
                
                auto new_pos = GetRandomPosition();
                
                player.stPosition = new_pos;
                
                auto gs_resp = CreateSVRespawnPlayer(m_oBuilder,
                                                     player.nUID,
                                                     player.stPosition.x,
                                                     player.stPosition.y);
                auto gs_event = CreateEvent(m_oBuilder,
                                            Events_SVRespawnPlayer,
                                            gs_resp.Union());
                m_oBuilder.Finish(gs_event);
                
                std::vector<uint8_t> packet(m_oBuilder.GetBufferPointer(),
                                            m_oBuilder.GetBufferPointer() +
                                            m_oBuilder.GetSize());
                m_aOutEvents.emplace(packet);
                
                m_oBuilder.Clear();
            }
        }
    }
}

std::queue<std::vector<uint8_t>>&
GameWorld::GetInEvents()
{
    return m_aInEvents;
}

std::queue<std::vector<uint8_t>>&
GameWorld::GetOutEvents()
{
    return m_aOutEvents;
}

Point2
GameWorld::GetRandomPosition()
{
    Point2 position;
    
    bool  bIsEngaged = false;
    do
    {
        position = m_oGameMap.GetRandomPosition();
        bIsEngaged = false;
        
        for(auto& player : m_aPlayers)
        {
            if(player.stPosition == position)
            {
                bIsEngaged = true;
                break;
            }
        }
        
        for(auto& item : m_aItems)
        {
            if(item.stPosition == position)
            {
                bIsEngaged = true;
                break;
            }
        }
        
        for(auto& constr : m_aConstructions)
        {
            if(constr.stPosition == position)
            {
                bIsEngaged = true;
                break;
            }
        }
    } while(bIsEngaged != false);
    
    return position;
}

std::vector<Player>::iterator
GameWorld::GetPlayerByUID(PlayerUID uid)
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
