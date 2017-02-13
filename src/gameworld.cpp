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
    flatbuffers::FlatBufferBuilder builder;
    std::vector<uint8_t> data;
        // spawn players
    for(auto& player : m_aPlayers)
    {
        Point2 spawn_point = GetRandomPosition();
        player.stPosition.x = spawn_point.x;
        player.stPosition.y = spawn_point.y;
        
        auto gs_spawn = GSNet::CreateGSSpawnPlayer(builder,
                                                   player.nUID,
                                                   player.stPosition.x,
                                                   player.stPosition.y);
        auto gs_event = GSNet::CreateGSEvent(builder,
                                             GSNet::GSEvents_GSSpawnPlayer,
                                             gs_spawn.Union());
        builder.Finish(gs_event);
        
        data.assign(builder.GetBufferPointer(),
                    builder.GetBufferPointer() + builder.GetSize());
        
        m_aOutEvents.emplace(data);
        builder.Clear();
    }
    
        // make a key
    auto pos = GetRandomPosition();
    Item item;
    item.eType = Item::Type::KEY;
    item.nUID = 0;
    item.stPosition.x = pos.x;
    item.stPosition.y = pos.y;
    m_aItems.push_back(item);
    
    auto gs_item = GSNet::CreateGSSpawnItem(builder,
                                            item.eType,
                                            item.nUID,
                                            item.stPosition.x,
                                            item.stPosition.y);
    auto gs_event = GSNet::CreateGSEvent(builder,
                                         GSNet::GSEvents_GSSpawnItem,
                                         gs_item.Union());
    builder.Finish(gs_event);
    
    data.assign(builder.GetBufferPointer(),
                builder.GetBufferPointer() + builder.GetSize());
    m_aOutEvents.emplace(data);
    builder.Clear();
    
        // make a door
    pos = GetRandomPosition();
    Construction door;
    door.eType = Construction::Type::DOOR;
    door.stPosition.x = pos.x;
    door.stPosition.y = pos.y;
    m_aConstructions.push_back(door);
    
    auto gs_constr = GSNet::CreateGSSpawnConstruction(builder,
                                                      door.eType,
                                                      door.stPosition.x,
                                                      door.stPosition.y);
    gs_event = GSNet::CreateGSEvent(builder,
                                    GSNet::GSEvents_GSSpawnConstruction,
                                    gs_constr.Union());
    builder.Finish(gs_event);
    
    data.assign(builder.GetBufferPointer(),
                builder.GetBufferPointer() + builder.GetSize());
    m_aOutEvents.emplace(data);
    builder.Clear();
    
        // spawn a swamp
    pos = GetRandomPosition();
    Construction swamp;
    swamp.eType = Construction::Type::SWAMP;
    swamp.stPosition.x = pos.x;
    swamp.stPosition.y = pos.y;
    m_aConstructions.push_back(swamp);
    
    gs_constr = GSNet::CreateGSSpawnConstruction(builder,
                                                 swamp.eType,
                                                 swamp.nUID,
                                                 swamp.stPosition.x,
                                                 swamp.stPosition.y);
    gs_event = GSNet::CreateGSEvent(builder,
                                    GSNet::GSEvents_GSSpawnConstruction,
                                    gs_constr.Union());
    builder.Finish(gs_event);
    
    data.assign(builder.GetBufferPointer(),
                builder.GetBufferPointer() + builder.GetSize());
    m_aOutEvents.emplace(data);
    builder.Clear();
    
        // make a monster
    pos = GetRandomPosition();
    Monster monster;
    monster.stPosition.x = pos.x;
    monster.stPosition.y = pos.y;
    monster.nUID = 0;
    m_aMonsters.push_back(monster);
    
    auto gs_monst = GSNet::CreateGSMovement(builder,
                                            monster.nUID,
                                            monster.stPosition.x,
                                            monster.stPosition.y);
    gs_event = GSNet::CreateGSEvent(builder,
                                    GSNet::GSEvents_GSSpawnMonster,
                                    gs_monst.Union());
    builder.Finish(gs_event);
    
    data.assign(builder.GetBufferPointer(),
                builder.GetBufferPointer() + builder.GetSize());
    m_aOutEvents.emplace(data);
    builder.Clear();
}

void
GameWorld::update(std::chrono::milliseconds ms)
{
    while(!m_aInEvents.empty()) // received packets are already validated
    {
        auto event = m_aInEvents.front();
        auto gs_event = GSNet::GetGSEvent(event.data());
        
        switch(gs_event->event_type())
        {
            case GSNet::GSEvents_CLMovement:
            {
                auto cl_mov = static_cast<const GSNet::CLMovement*>(gs_event->event());
                auto player = GetPlayerByUID(cl_mov->player_uid());
                
                    // apply movement changes
                player->stPosition.x = cl_mov->x();
                player->stPosition.y = cl_mov->y();
                
                auto gs_mov = GSNet::CreateGSMovement(m_oBuilder,
                                                      player->nUID,
                                                      player->stPosition.x,
                                                      player->stPosition.y);
                auto gs_ev = GSNet::CreateGSEvent(m_oBuilder,
                                                  GSNet::GSEvents_GSMovement,
                                                  gs_mov.Union());
                m_oBuilder.Finish(gs_ev);
                
                std::vector<uint8_t> packet(m_oBuilder.GetBufferPointer(),
                                            m_oBuilder.GetBufferPointer() +
                                            m_oBuilder.GetSize());
                m_aOutEvents.emplace(packet);
                
                m_oBuilder.Clear();
                break;
            }
                
            case GSNet::GSEvents_CLTakeItem:
            {
                auto cl_take = static_cast<const GSNet::CLTakeItem*>(gs_event->event());
                auto player = GetPlayerByUID(cl_take->player_uid());
                auto item = std::find_if(m_aItems.begin(),
                                         m_aItems.end(),
                                         [cl_take](Item& it)
                                         {
                                             return it.nUID == cl_take->item_uid();
                                         });
                if(item != m_aItems.end() &&
                   item->nCarrierID == 0)
                {
                    item->nCarrierID = player->nUID;
                    
                    auto gs_take = GSNet::CreateGSTakeItem(m_oBuilder,
                                                           player->nUID,
                                                           item->nUID);
                    auto gs_ev = GSNet::CreateGSEvent(m_oBuilder,
                                                      GSNet::GSEvents_GSTakeItem,
                                                      gs_take.Union());
                    m_oBuilder.Finish(gs_ev);
                    
                    std::vector<uint8_t> packet(m_oBuilder.GetBufferPointer(),
                                                m_oBuilder.GetBufferPointer() +
                                                m_oBuilder.GetSize());
                    m_aOutEvents.emplace(packet);
                    
                    m_oBuilder.Clear();
                    break;
                }
                
            }
                
            case GSNet::GSEvents_CLPlayerEscapeDrown:
            {
                auto cl_esc = static_cast<const GSNet::CLPlayerEscapeDrown*>(gs_event->event());
                auto player = GetPlayerByUID(cl_esc->player_uid());
                
                player->eState = Player::State::WALKING;
                
                    // notify players that player escaped
                auto gs_esc = GSNet::CreateGSPlayerEscapeDrown(m_oBuilder,
                                                               player->nUID);
                auto gs_ev = GSNet::CreateGSEvent(m_oBuilder,
                                                  GSNet::GSEvents_GSPlayerEscapeDrown,
                                                  gs_esc.Union());
                m_oBuilder.Finish(gs_ev);
                std::vector<uint8_t> packet(m_oBuilder.GetBufferPointer(),
                                            m_oBuilder.GetBufferPointer() +
                                            m_oBuilder.GetSize());
                m_aOutEvents.emplace(packet);
                m_oBuilder.Clear();
                
                    // move player away from swamp
                    // FIXME: dangerous code
                auto& map = m_oGameMap.GetMap();
                if(map[player->stPosition.x-1][player->stPosition.y] == GameMap::MapBlockType::NOBLOCK)
                {
                    player->stPosition.x--;
                    auto gs_mov = GSNet::CreateGSMovement(m_oBuilder,
                                                          player->nUID,
                                                          player->stPosition.x,
                                                          player->stPosition.y);
                    auto gs_ev = GSNet::CreateGSEvent(m_oBuilder,
                                                      GSNet::GSEvents_GSMovement,
                                                      gs_mov.Union());
                    m_oBuilder.Finish(gs_ev);
                    std::vector<uint8_t> packet(m_oBuilder.GetBufferPointer(),
                                                m_oBuilder.GetBufferPointer() +
                                                m_oBuilder.GetSize());
                    m_aOutEvents.emplace(packet);
                    m_oBuilder.Clear();
                }
                
                break;
            }
            default:
                assert(false);
                break;
        }
        
        m_aInEvents.pop(); // remove packet
    }
    
        // check that player stepped into swamp
    for(auto& player : m_aPlayers)
    {
        for(auto& constr : m_aConstructions)
        {
            if(constr.eType == Construction::Type::SWAMP &&
               player.stPosition == constr.stPosition)
            {
                    // player is drowning!
                player.eState = Player::State::DROWNING;
                
                auto gs_drown = GSNet::CreateGSPlayerDrown(m_oBuilder,
                                                           player.nUID);
                auto gs_event = GSNet::CreateGSEvent(m_oBuilder,
                                                     GSNet::GSEvents_GSPlayerDrown,
                                                     gs_drown.Union());
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
