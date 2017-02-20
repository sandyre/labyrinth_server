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
m_eState(State::RUNNING),
m_stSettings(sets),
m_aPlayers(players)
{
}

void
GameWorld::generate_map()
{
    m_oGameMap = GameMap(m_stSettings.stGMSettings);
}

GameWorld::State
GameWorld::GetState()
{
    return m_eState;
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
        player.nHP = player.nHPMax;
        
        auto gs_spawn = CreateSVSpawnPlayer(m_oBuilder,
                                            player.nUID,
                                            player.stPosition.x,
                                            player.stPosition.y,
                                            player.nHP,
                                            player.nHPMax);
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
    Item key = m_oItemFactory.createItem(Item::Type::KEY);
    key.stPosition = pos;
    m_aItems.push_back(key);
    
    auto gs_item = CreateSVSpawnItem(m_oBuilder,
                                     key.nUID,
                                     ItemType_KEY,
                                     key.stPosition.x,
                                     key.stPosition.y);
    auto gs_event = CreateEvent(m_oBuilder,
                                Events_SVSpawnItem,
                                gs_item.Union());
    m_oBuilder.Finish(gs_event);
    
    data.assign(m_oBuilder.GetBufferPointer(),
                m_oBuilder.GetBufferPointer() + m_oBuilder.GetSize());
    m_aOutEvents.emplace(data);
    m_oBuilder.Clear();
    
        // make a door
    pos = GetRandomPosition();
    Construction door = m_oConstrFactory.createConstruction(Construction::Type::DOOR);
    door.stPosition = pos;
    m_aConstructions.push_back(door);
    
    auto gs_constr = CreateSVSpawnConstr(m_oBuilder,
                                         door.nUID,
                                         ConstrType_DOOR,
                                         door.stPosition.x,
                                         door.stPosition.y);
    gs_event = CreateEvent(m_oBuilder,
                           Events_SVSpawnConstr,
                           gs_constr.Union());
    m_oBuilder.Finish(gs_event);
    
    data.assign(m_oBuilder.GetBufferPointer(),
                m_oBuilder.GetBufferPointer() + m_oBuilder.GetSize());
    m_aOutEvents.emplace(data);
    m_oBuilder.Clear();
    
        // make a sword
    pos = GetRandomPosition();
    Item sword = m_oItemFactory.createItem(Item::Type::SWORD);
    sword.stPosition = pos;
    m_aItems.push_back(sword);
    
    gs_item = CreateSVSpawnItem(m_oBuilder,
                                sword.nUID,
                                ItemType_SWORD,
                                sword.stPosition.x,
                                sword.stPosition.y);
    gs_event = CreateEvent(m_oBuilder,
                                Events_SVSpawnItem,
                                gs_item.Union());
    m_oBuilder.Finish(gs_event);
    
    data.assign(m_oBuilder.GetBufferPointer(),
                m_oBuilder.GetBufferPointer() + m_oBuilder.GetSize());
    m_aOutEvents.emplace(data);
    m_oBuilder.Clear();
    
        // make graveyard
    pos = GetRandomPosition();
    Construction grave = m_oConstrFactory.createConstruction(Construction::Type::GRAVEYARD);
    grave.stPosition = pos;
    m_aConstructions.push_back(grave);
    
    gs_constr = CreateSVSpawnConstr(m_oBuilder,
                                    grave.nUID,
                                    ConstrType_GRAVEYARD,
                                    grave.stPosition.x,
                                    grave.stPosition.y);
    gs_event = CreateEvent(m_oBuilder,
                           Events_SVSpawnConstr,
                           gs_constr.Union());
    m_oBuilder.Finish(gs_event);
    
    data.assign(m_oBuilder.GetBufferPointer(),
                m_oBuilder.GetBufferPointer() + m_oBuilder.GetSize());
    m_aOutEvents.emplace(data);
    m_oBuilder.Clear();
    
        // make swamp
    pos = GetRandomPosition();
    Construction swamp = m_oConstrFactory.createConstruction(Construction::Type::SWAMP);
    swamp.stPosition = pos;
    m_aConstructions.push_back(swamp);
    
    gs_constr = CreateSVSpawnConstr(m_oBuilder,
                                    swamp.nUID,
                                    ConstrType_SWAMP,
                                    swamp.stPosition.x,
                                    swamp.stPosition.y);
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
                
            case Events_CLActionItem:
            {
                auto cl_item = static_cast<const CLActionItem*>(gs_event->event());
                auto player = GetPlayerByUID(cl_item->player_uid());
                auto item = std::find_if(m_aItems.begin(),
                                         m_aItems.end(),
                                         [cl_item](Item& it)
                                         {
                                             return it.nUID == cl_item->item_uid();
                                         });
                
                    // invalid item, skip event
                if(item == m_aItems.end())
                    continue;
                
                switch (cl_item->act_type())
                {
                    case ActionItemType_TAKE:
                    {
                        if(item->nCarrierID == 0) // player takes item
                        {
                            item->nCarrierID = player->nUID;
                            
                            auto gs_take = CreateSVActionItem(m_oBuilder,
                                                              player->nUID,
                                                              item->nUID,
                                                              ActionItemType_TAKE);
                            auto gs_event = CreateEvent(m_oBuilder,
                                                        Events_SVActionItem,
                                                        gs_take.Union());
                            m_oBuilder.Finish(gs_event);
                            
                            std::vector<uint8_t> packet(m_oBuilder.GetBufferPointer(),
                                                        m_oBuilder.GetBufferPointer() +
                                                        m_oBuilder.GetSize());
                            m_aOutEvents.emplace(packet);
                            
                            m_oBuilder.Clear();
                        }
                        
                        break;
                    }
                        
                    case ActionItemType_DROP:
                    {
                        item->nCarrierID = 0;
                        item->stPosition = player->stPosition;
                        
                        auto gs_drop = CreateSVActionItem(m_oBuilder,
                                                          player->nUID,
                                                          item->nUID,
                                                          ActionItemType_DROP);
                        auto gs_event = CreateEvent(m_oBuilder,
                                                    Events_SVActionItem,
                                                    gs_drop.Union());
                        m_oBuilder.Finish(gs_event);
                        
                        std::vector<uint8_t> packet(m_oBuilder.GetBufferPointer(),
                                                    m_oBuilder.GetBufferPointer() +
                                                    m_oBuilder.GetSize());
                        m_aOutEvents.emplace(packet);
                        
                        m_oBuilder.Clear();
                        
                        break;
                    }
                        
                    default:
                        assert(false);
                        break;
                }
                
                break;
            }
                
            case Events_CLActionSwamp:
            {
                auto cl_swamp = static_cast<const CLActionSwamp*>(gs_event->event());
                auto player = GetPlayerByUID(cl_swamp->player_uid());
                
                if(cl_swamp->status() == GameEvent::ActionSwampStatus_ESCAPED)
                {
                    player->eState = Player::State::WALKING;
                    player->nTimer = 0;
                    
                        // notify that player escaped
                    auto sv_esc = CreateSVActionSwamp(m_oBuilder,
                                                      player->nUID,
                                                      ActionSwampStatus_ESCAPED);
                    auto sv_event = CreateEvent(m_oBuilder,
                                                Events_SVActionSwamp,
                                                sv_esc.Union());
                    m_oBuilder.Finish(sv_event);
                    
                    std::vector<uint8_t> packet(m_oBuilder.GetBufferPointer(),
                                                m_oBuilder.GetBufferPointer() +
                                                m_oBuilder.GetSize());
                    m_aOutEvents.emplace(packet);
                    
                    m_oBuilder.Clear();
                    
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
                    sv_event = CreateEvent(m_oBuilder,
                                           Events_SVActionMove,
                                           sv_move.Union());
                    m_oBuilder.Finish(sv_event);
                    
                    packet.assign(m_oBuilder.GetBufferPointer(),
                                  m_oBuilder.GetBufferPointer() +
                                  m_oBuilder.GetSize());
                    m_aOutEvents.emplace(packet);
                    
                    m_oBuilder.Clear();
                }
                
                break;
            }
                
            case Events_CLActionDuel:
            {
                auto cl_duel = static_cast<const CLActionDuel*>(gs_event->event());
                auto player1 = GetPlayerByUID(cl_duel->player1_uid());
                auto player2 = GetPlayerByUID(cl_duel->player2_uid());
                
                switch(cl_duel->type())
                {
                    case ActionDuelType_ATTACK:
                    {
                        player2->nHP -= player1->nDamage;
                        
                        auto sv_duel_att = CreateSVActionDuel(m_oBuilder,
                                                              player1->nUID, // who attacks goes first
                                                              player2->nUID, // attacked goes second
                                                              ActionDuelType_ATTACK);
                        auto sv_event = CreateEvent(m_oBuilder,
                                                    Events_SVActionDuel,
                                                    sv_duel_att.Union());
                        m_oBuilder.Finish(sv_event);
                        
                        std::vector<uint8_t> packet(m_oBuilder.GetBufferPointer(),
                                                    m_oBuilder.GetBufferPointer() +
                                                    m_oBuilder.GetSize());
                        m_aOutEvents.emplace(packet);
                        
                        m_oBuilder.Clear();
                        
                        if(player2->nHP <= 0)
                        {
                            player2->nHP = player2->nHPMax; // set hp to default
                            
                            player2->eState = Player::State::DEAD; // will be respawned
                            player1->eState = Player::State::WALKING; // can now walk
                            
                                // duel ends, ENEMY dead
                            auto sv_duel_end = CreateSVActionDuel(m_oBuilder,
                                                                  player1->nUID, // killer first
                                                                  player2->nUID, // who died second
                                                                  ActionDuelType_KILL);
                            sv_event = CreateEvent(m_oBuilder,
                                                   Events_SVActionDuel,
                                                   sv_duel_end.Union());
                            m_oBuilder.Finish(sv_event);
                            
                            packet.assign(m_oBuilder.GetBufferPointer(),
                                          m_oBuilder.GetBufferPointer() +
                                          m_oBuilder.GetSize());
                            m_aOutEvents.emplace(packet);
                            
                            m_oBuilder.Clear();
                        }
                        
                        break;
                    }
                        
                    case ActionDuelType_ESCAPE:
                    {
                            // set players new state
                        player1->eState = Player::State::WALKING; // player escaped
                        player2->eState = Player::State::WALKING;
                        
                        auto sv_escape = CreateSVActionDuel(m_oBuilder,
                                                            player1->nUID, // who escaped
                                                            player2->nUID,
                                                            ActionDuelType_ESCAPE);
                        auto sv_event = CreateEvent(m_oBuilder,
                                                    Events_SVActionDuel,
                                                    sv_escape.Union());
                        m_oBuilder.Finish(sv_event);
                        
                        std::vector<uint8_t> packet(m_oBuilder.GetBufferPointer(),
                                                    m_oBuilder.GetBufferPointer() +
                                                    m_oBuilder.GetSize());
                        m_aOutEvents.emplace(packet);
                        
                        m_oBuilder.Clear();
                        
                        break;
                    }
                        
                    default:
                        assert(false);
                        break;
                }
                
                break;
            }
                
            default:
                assert(false);
                break;
        }
        
        m_aInEvents.pop(); // remove packet
    }
        // check that player can fight
    for(auto& x : m_aPlayers)
    {
        for(auto& y : m_aPlayers)
        {
            if(x.nUID != y.nUID)
            {
                if(x.eState == Player::State::WALKING &&
                   y.eState == Player::State::WALKING &&
                   x.stPosition == y.stPosition)
                {
                        // battle begins
                    x.eState = Player::State::DUEL;
                    y.eState = Player::State::DUEL;
                    
                    auto sv_duel = CreateSVActionDuel(m_oBuilder,
                                                      x.nUID,
                                                      y.nUID,
                                                      ActionDuelType_STARTED);
                    auto sv_event = CreateEvent(m_oBuilder,
                                                Events_SVActionDuel,
                                                sv_duel.Union());
                    m_oBuilder.Finish(sv_event);
                    
                    std::vector<uint8_t> packet(m_oBuilder.GetBufferPointer(),
                                                m_oBuilder.GetBufferPointer() +
                                                m_oBuilder.GetSize());
                    m_aOutEvents.emplace(packet);
                    
                    m_oBuilder.Clear();
                }
            }
        }
    }
    
        // timer for players in swamp
    for(auto& player : m_aPlayers)
    {
        if(player.eState == Player::State::SWAMP)
        {
            player.nTimer += ms.count();
            
            if(player.nTimer > 8000)
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
                player.nHP = player.nHPMax;
                
                player.eState = Player::State::WALKING;
                
                auto grave = std::find_if(m_aConstructions.begin(),
                                          m_aConstructions.end(),
                                          [](Construction constr)
                                          {
                                              return constr.eType == Construction::Type::GRAVEYARD;
                                          });
                
                player.stPosition = grave->stPosition;
                
                auto gs_resp = CreateSVRespawnPlayer(m_oBuilder,
                                                     player.nUID,
                                                     player.stPosition.x,
                                                     player.stPosition.y,
                                                     player.nHP,
                                                     player.nHPMax);
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
    
        // check that game is over
    auto key = std::find_if(m_aItems.begin(),
                            m_aItems.end(),
                            [](Item& it)
                            {
                                return it.eType == Item::Type::KEY;
                            });
    auto door = std::find_if(m_aConstructions.begin(),
                             m_aConstructions.end(),
                             [](Construction& constr)
                             {
                                 return constr.eType == Construction::Type::DOOR;
                             });
    for(auto& player : m_aPlayers)
    {
        if(player.nUID == key->nCarrierID &&
           player.stPosition == door->stPosition)
        {
                // game is over
            m_eState = GameWorld::State::FINISHED;
            
            auto gs_go = CreateSVGameEnd(m_oBuilder,
                                         player.nUID);
            auto gs_event = CreateEvent(m_oBuilder,
                                        Events_SVGameEnd,
                                        gs_go.Union());
            m_oBuilder.Finish(gs_event);
            
            std::vector<uint8_t> packet(m_oBuilder.GetBufferPointer(),
                                        m_oBuilder.GetBufferPointer() +
                                        m_oBuilder.GetSize());
            m_aOutEvents.emplace(packet);
            
            m_oBuilder.Clear();
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
