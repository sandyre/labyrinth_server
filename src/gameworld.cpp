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
        // spawn players
    for(auto& player : m_aPlayers)
    {
        player.CreateHero(); // init hero first
        
        Point2 spawn_point = GetRandomPosition();
        
        player()->MoveTo(spawn_point);
        
        auto gs_spawn = CreateSVSpawnPlayer(m_oBuilder,
                                            player.GetUID(),
                                            player()->GetPosition().x,
                                            player()->GetPosition().y,
                                            player()->GetHealth(),
                                            player()->GetMHealth());
        auto gs_event = CreateMessage(m_oBuilder,
                                      Events_SVSpawnPlayer,
                                      gs_spawn.Union());
        m_oBuilder.Finish(gs_event);
        
        PushEventAndClear();
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
    auto gs_event = CreateMessage(m_oBuilder,
                                Events_SVSpawnItem,
                                gs_item.Union());
    m_oBuilder.Finish(gs_event);
    PushEventAndClear();
    
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
    gs_event = CreateMessage(m_oBuilder,
                           Events_SVSpawnConstr,
                           gs_constr.Union());
    m_oBuilder.Finish(gs_event);
    PushEventAndClear();
    
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
    gs_event = CreateMessage(m_oBuilder,
                                Events_SVSpawnItem,
                                gs_item.Union());
    m_oBuilder.Finish(gs_event);
    PushEventAndClear();
    
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
    gs_event = CreateMessage(m_oBuilder,
                           Events_SVSpawnConstr,
                           gs_constr.Union());
    m_oBuilder.Finish(gs_event);
    PushEventAndClear();
    
        // make a monster
    pos = GetRandomPosition();
    Monster monster = m_oMonsterFactory.createMonster();
    monster.stPosition = pos;
    m_aMonsters.push_back(monster);
    
    auto gs_monster = CreateSVSpawnMonster(m_oBuilder,
                                           monster.nUID,
                                           monster.stPosition.x,
                                           monster.stPosition.y,
                                           monster.nHP,
                                           monster.nMaxHP);
    
    gs_event = CreateMessage(m_oBuilder,
                           Events_SVSpawnMonster,
                           gs_monster.Union());
    m_oBuilder.Finish(gs_event);
    PushEventAndClear();
}

void
GameWorld::update(std::chrono::milliseconds ms)
{
    ApplyInputEvents();
    UpdateMonsters(ms);
    UpdatePlayers(ms);
    UpdateWorld(ms);
    
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
        if(player.GetUID() == key->nCarrierID &&
           player()->GetPosition() == door->stPosition)
        {
                // game is over
            m_eState = GameWorld::State::FINISHED;
            
            auto gs_go = CreateSVGameEnd(m_oBuilder,
                                         player.GetUID());
            auto gs_event = CreateMessage(m_oBuilder,
                                        Events_SVGameEnd,
                                        gs_go.Union());
            m_oBuilder.Finish(gs_event);
            PushEventAndClear();
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
            if(player() != nullptr &&
               player()->GetPosition() == position)
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


int32_t
GameWorld::FindPlayerByUID(PlayerUID uid)
{
    for(int32_t i = 0;
        i < m_aPlayers.size();
        ++i)
    {
        if(m_aPlayers[i].GetUID() == uid)
        {
            return i;
        }
    }
    
    return -1;
}

void
GameWorld::PushEventAndClear()
{
    std::vector<uint8_t> packet(m_oBuilder.GetBufferPointer(),
                                m_oBuilder.GetBufferPointer() +
                                m_oBuilder.GetSize());
    m_aOutEvents.emplace(packet);
    
    m_oBuilder.Clear();
}

void
GameWorld::ProcessDuelEvent(const GameEvent::CLActionDuel* cl_duel)
{
    auto& player1 = m_aPlayers[FindPlayerByUID(cl_duel->target1_uid())];
    
    if(cl_duel->act_type() == ActionDuelType_STARTED)
    {
        auto& player2 = m_aPlayers[FindPlayerByUID(cl_duel->target2_uid())];
        
        player1()->SetState(Hero::State::DUEL_HERO);
        player2()->SetState(Hero::State::DUEL_HERO);
        
        auto sv_duel = CreateSVActionDuel(m_oBuilder,
                                          player1.GetUID(),
                                          ActionDuelTarget_PLAYER,
                                          player2.GetUID(),
                                          ActionDuelTarget_PLAYER,
                                          ActionDuelType_STARTED);
        auto sv_event = CreateMessage(m_oBuilder,
                                    Events_SVActionDuel,
                                    sv_duel.Union());
        m_oBuilder.Finish(sv_event);
        
        PushEventAndClear();
    }
    else if(cl_duel->act_type() == ActionDuelType_ATTACK)
    {
            // vs player handling
        if(cl_duel->target2_type() == ActionDuelTarget_PLAYER)
        {
            auto& player2 = m_aPlayers[FindPlayerByUID(cl_duel->target2_uid())];
            
            player2()->SetHealth(player2()->GetHealth() -
                                 player1()->GetDamage());
            
            auto sv_duel_att = CreateSVActionDuel(m_oBuilder,
                                                  player1.GetUID(),
                                                  ActionDuelTarget_PLAYER,
                                                  player2.GetUID(),
                                                  ActionDuelTarget_PLAYER,
                                                  ActionDuelType_ATTACK,
                                                  cl_duel->damage());
            auto sv_event = CreateMessage(m_oBuilder,
                                        Events_SVActionDuel,
                                        sv_duel_att.Union());
            m_oBuilder.Finish(sv_event);
            PushEventAndClear();
            
            if(player2()->GetHealth() <= 0)
            {
                player2()->Die(5.0); // player launches timer and will request respawn
                    // drop items
                for(auto& item : m_aItems)
                {
                    if(player2.GetUID() == item.nCarrierID)
                    {
                        item.stPosition = player2()->GetPosition();
                        item.nCarrierID = 0;
                        
                        auto sv_item_drop = CreateSVActionItem(m_oBuilder,
                                                               player2.GetUID(),
                                                               item.nUID,
                                                               ActionItemType_DROP);
                        auto sv_event = CreateMessage(m_oBuilder,
                                                    Events_SVActionItem,
                                                    sv_item_drop.Union());
                        m_oBuilder.Finish(sv_event);
                        PushEventAndClear();
                    }
                }
                
                    // duel ends
                player1()->SetState(Hero::State::WALKING);
                
                auto sv_duel_end = CreateSVActionDuel(m_oBuilder,
                                                      player1.GetUID(),
                                                      ActionDuelTarget_PLAYER,
                                                      player2.GetUID(),
                                                      ActionDuelTarget_PLAYER,
                                                      ActionDuelType_KILL);
                auto sv_event = CreateMessage(m_oBuilder,
                                            Events_SVActionDuel,
                                            sv_duel_end.Union());
                m_oBuilder.Finish(sv_event);
                PushEventAndClear();
            }
        }
            // vs monster handling
        else if(cl_duel->target2_type() == ActionDuelTarget_MONSTER)
        {
            auto monster = std::find_if(m_aMonsters.begin(),
                                        m_aMonsters.end(),
                                        [cl_duel](Monster& monst)
                                        {
                                            return monst.nUID == cl_duel->target2_uid();
                                        });
            
                // player only can attack, but act_type() validation needed
            monster->nHP -= cl_duel->damage();
            
            auto sv_duel_att = CreateSVActionDuel(m_oBuilder,
                                                  player1.GetUID(),
                                                  ActionDuelTarget_PLAYER,
                                                  monster->nUID,
                                                  ActionDuelTarget_MONSTER,
                                                  ActionDuelType_ATTACK,
                                                  cl_duel->damage());
            auto sv_event = CreateMessage(m_oBuilder,
                                        Events_SVActionDuel,
                                        sv_duel_att.Union());
            m_oBuilder.Finish(sv_event);
            PushEventAndClear();
            
            if(monster->nHP <= 0)
            {
                player1()->SetState(Hero::State::WALKING);
                player1()->EndDuel();
                monster->eState = Monster::State::DEAD;
                
                    // duel ends
                auto sv_duel_end = CreateSVActionDuel(m_oBuilder,
                                                      player1.GetUID(),
                                                      ActionDuelTarget_PLAYER,
                                                      monster->nUID,
                                                      ActionDuelTarget_MONSTER,
                                                      ActionDuelType_KILL);
                auto sv_event = CreateMessage(m_oBuilder,
                                            Events_SVActionDuel,
                                            sv_duel_end.Union());
                m_oBuilder.Finish(sv_event);
                PushEventAndClear();
            }
        }
    }
    else
    {
        assert(false);
    }
}

    // FIXME: undone (not implemented, swamps disabled)
void
GameWorld::ProcessSwampEvent(const GameEvent::CLActionSwamp* cl_swamp)
{
//    auto& player = m_aPlayers[FindPlayerByUID(cl_swamp->player_uid())];
//    
//    if(cl_swamp->status() == GameEvent::ActionSwampStatus_ESCAPED)
//    {
//        player()->SetState(Hero::State::WALKING);
//        
//            // notify that player escaped
//        auto sv_esc = CreateSVActionSwamp(m_oBuilder,
//                                          player.GetUID(),
//                                          ActionSwampStatus_ESCAPED);
//        auto sv_event = CreateMessage(m_oBuilder,
//                                    Events_SVActionSwamp,
//                                    sv_esc.Union());
//        m_oBuilder.Finish(sv_event);
//        PushEventAndClear();
//        
////            // move player away from swamp
////        auto& map = m_oGameMap.GetMap();
////        if(map[player()->GetPosition().x-1][player()->GetPosition().y] ==
////           GameMap::MapBlockType::NOBLOCK)
////        {
////            player.stPosition.x--;
////        }
////        else if(map[player.stPosition.x+1][player.stPosition.y] ==
////                GameMap::MapBlockType::NOBLOCK)
////        {
////            player.stPosition.x++;
////        }
////        else if(map[player.stPosition.x][player.stPosition.y-1] ==
////                GameMap::MapBlockType::NOBLOCK)
////        {
////            player.stPosition.y--;
////        }
////        else if(map[player.stPosition.x+1][player.stPosition.y+1] ==
////                GameMap::MapBlockType::NOBLOCK)
////        {
////            player.stPosition.y++;
////        }
//        
//        auto sv_move = CreateSVActionMove(m_oBuilder,
//                                          player.nUID,
//                                          ActionMoveTarget_PLAYER,
//                                          player.stPosition.x,
//                                          player.stPosition.y);
//        sv_event = CreateMessage(m_oBuilder,
//                               Events_SVActionMove,
//                               sv_move.Union());
//        m_oBuilder.Finish(sv_event);
//        PushEventAndClear();
//    }
}

void
GameWorld::ProcessItemEvent(const GameEvent::CLActionItem* cl_item)
{
    auto& player = m_aPlayers[FindPlayerByUID(cl_item->player_uid())];
    
    auto item = std::find_if(m_aItems.begin(),
                             m_aItems.end(),
                             [cl_item](Item& it)
                             {
                                 return it.nUID == cl_item->item_uid();
                             });
    
        // invalid item, skip event
    if(item == m_aItems.end())
        return;
    
    switch (cl_item->act_type())
    {
        case ActionItemType_TAKE:
        {
            if(item->nCarrierID == 0) // player takes item
            {
                item->nCarrierID = player.GetUID();
                player()->TakeItem(*item);
                
                auto gs_take = CreateSVActionItem(m_oBuilder,
                                                  player.GetUID(),
                                                  item->nUID,
                                                  ActionItemType_TAKE);
                auto gs_event = CreateMessage(m_oBuilder,
                                            Events_SVActionItem,
                                            gs_take.Union());
                m_oBuilder.Finish(gs_event);
                PushEventAndClear();
            }
            
            break;
        }
            
        case ActionItemType_DROP:
        {
            item->nCarrierID = 0;
            item->stPosition = player()->GetPosition();
            
            auto gs_drop = CreateSVActionItem(m_oBuilder,
                                              player.GetUID(),
                                              item->nUID,
                                              ActionItemType_DROP);
            auto gs_event = CreateMessage(m_oBuilder,
                                        Events_SVActionItem,
                                        gs_drop.Union());
            m_oBuilder.Finish(gs_event);
            PushEventAndClear();
            break;
        }
            
        default:
            assert(false);
            break;
    }
}

void
GameWorld::ProcessMoveEvent(const GameEvent::CLActionMove* cl_mov)
{
    auto& player = m_aPlayers[FindPlayerByUID(cl_mov->target_uid())];
    
        // check that player CAN walk
    if(!(player()->GetStatus() & Hero::Status::MOVABLE))
        return;
    
        // apply movement changes
    player()->SetPosition(cl_mov->x(),
                          cl_mov->y());
    
    auto gs_mov = CreateSVActionMove(m_oBuilder,
                                     player.GetUID(),
                                     ActionMoveTarget_PLAYER,
                                     player()->GetPosition().x,
                                     player()->GetPosition().y);
    auto gs_ev = CreateMessage(m_oBuilder,
                               Events_SVActionMove,
                               gs_mov.Union());
    m_oBuilder.Finish(gs_ev);
    PushEventAndClear();
}

void
GameWorld::UpdateMonsters(std::chrono::milliseconds ms)
{
    for(auto& monster : m_aMonsters)
    {
        if(monster.eState == Monster::WAITING)
        {
            for(auto& player : m_aPlayers)
            {
                if(std::abs(monster.stPosition.x - player()->GetPosition().x) <= monster.nVision &&
                   std::abs(monster.stPosition.y - player()->GetPosition().y) <= monster.nVision)
                {
                    monster.eState = Monster::State::CHARGING;
                    monster.nChargingUID = player.GetUID();
                }
            }
        }
        else if(monster.eState == Monster::CHARGING)
        {
            auto& player = m_aPlayers[FindPlayerByUID(monster.nChargingUID)];
            
                // duel starts
            if((player()->GetStatus() & Hero::Status::DUELABLE) &&
               monster.stPosition == player()->GetPosition())
            {
                monster.eState = Monster::State::DUEL;
                player()->StartDuel(monster.nUID,
                                    false);
                auto sv_duel = CreateSVActionDuel(m_oBuilder,
                                                  player.GetUID(),
                                                  ActionDuelTarget_PLAYER,
                                                  monster.nUID,
                                                  ActionDuelTarget_MONSTER,
                                                  ActionDuelType_STARTED);
                auto sv_event = CreateMessage(m_oBuilder,
                                            Events_SVActionDuel,
                                            sv_duel.Union());
                m_oBuilder.Finish(sv_event);
                PushEventAndClear();
                continue; // go to the next monster
            }
            
            if(std::abs(monster.stPosition.x - player()->GetPosition().x) < monster.nChargeRadius &&
               std::abs(monster.stPosition.y - player()->GetPosition().y) < monster.nChargeRadius)
            {
                monster.nTimer += ms.count();
                
                if(monster.nTimer > monster.nMoveCD)
                {
                    monster.nTimer = 0;
                    
                    auto m_pos = monster.stPosition;
                    if(m_pos.x > player()->GetPosition().x &&
                       m_oGameMap[m_pos.x-1][m_pos.y] == GameMap::MapBlockType::NOBLOCK)
                    {
                        --m_pos.x;
                    }
                    else if(m_pos.x < player()->GetPosition().x &&
                            m_oGameMap[m_pos.x+1][m_pos.y] == GameMap::MapBlockType::NOBLOCK)
                    {
                        ++m_pos.x;
                    }
                    else if(m_pos.y > player()->GetPosition().y &&
                            m_oGameMap[m_pos.x][m_pos.y-1] == GameMap::MapBlockType::NOBLOCK)
                    {
                        --m_pos.y;
                    }
                    else if(m_pos.y < player()->GetPosition().y &&
                            m_oGameMap[m_pos.x][m_pos.y+1] == GameMap::MapBlockType::NOBLOCK)
                    {
                        ++m_pos.y;
                    }
                        // no direct way to player, go somewhere
                    else
                    {
                        if(m_oGameMap[m_pos.x-1][m_pos.y] == GameMap::MapBlockType::NOBLOCK)
                        {
                            --m_pos.x;
                        }
                        else if(m_oGameMap[m_pos.x+1][m_pos.y] == GameMap::MapBlockType::NOBLOCK)
                        {
                            ++m_pos.x;
                        }
                        else if(m_oGameMap[m_pos.x][m_pos.y-1] == GameMap::MapBlockType::NOBLOCK)
                        {
                            --m_pos.y;
                        }
                        else if(m_oGameMap[m_pos.x][m_pos.y+1] == GameMap::MapBlockType::NOBLOCK)
                        {
                            ++m_pos.y;
                        }
                    }
                    
                    monster.stPosition = m_pos;
                    
                    auto sv_move = CreateSVActionMove(m_oBuilder,
                                                      monster.nUID,
                                                      ActionMoveTarget_MONSTER,
                                                      monster.stPosition.x,
                                                      monster.stPosition.y);
                    auto sv_event = CreateMessage(m_oBuilder,
                                                Events_SVActionMove,
                                                sv_move.Union());
                    m_oBuilder.Finish(sv_event);
                    PushEventAndClear();
                }
            }
            else
            {
                monster.eState = Monster::State::WAITING;
                monster.nChargingUID = 0;
                monster.nTimer = 0;
            }
        }
        else if(monster.eState == Monster::State::DUEL)
        {
            monster.nTimer += ms.count();
            if(monster.nTimer > monster.nAttackCD)
            {
                monster.nTimer = 0;
                auto& player = m_aPlayers[FindPlayerByUID(monster.nChargingUID)];
                
                auto sv_duel = CreateSVActionDuel(m_oBuilder,
                                                  monster.nUID,
                                                  ActionDuelTarget_MONSTER,
                                                  monster.nChargingUID,
                                                  ActionDuelTarget_PLAYER,
                                                  ActionDuelType_ATTACK,
                                                  monster.nDamage);
                auto sv_event = CreateMessage(m_oBuilder,
                                            Events_SVActionDuel,
                                            sv_duel.Union());
                m_oBuilder.Finish(sv_event);
                PushEventAndClear();
                
                player()->SetHealth(player()->GetHealth() - monster.nDamage);
                
                if(player()->GetHealth() <= 0)
                {
                    player()->Die(5.0);
                    
                        // drop items
                    for(auto& item : m_aItems)
                    {
                        if(player.GetUID() == item.nCarrierID)
                        {
                            item.stPosition = player()->GetPosition();
                            item.nCarrierID = 0;
                            auto sv_item_drop = CreateSVActionItem(m_oBuilder,
                                                                   player.GetUID(),
                                                                   item.nUID,
                                                                   ActionItemType_DROP);
                            auto gs_event = CreateMessage(m_oBuilder,
                                                        Events_SVActionItem,
                                                        sv_item_drop.Union());
                            m_oBuilder.Finish(gs_event);
                            PushEventAndClear();
                        }
                    }
                    
                    monster.eState = Monster::State::WAITING;
                    
                        // duel ends, PLAYER dead
                    auto sv_duel_end = CreateSVActionDuel(m_oBuilder,
                                                          monster.nUID,
                                                          ActionDuelTarget_MONSTER,
                                                          player.GetUID(), // who died second
                                                          ActionDuelTarget_PLAYER,
                                                          ActionDuelType_KILL);
                    auto sv_event = CreateMessage(m_oBuilder,
                                                Events_SVActionDuel,
                                                sv_duel_end.Union());
                    m_oBuilder.Finish(sv_event);
                    PushEventAndClear();
                }
            }
        }
    }
}

void
GameWorld::UpdatePlayers(std::chrono::milliseconds delta)
{
    for(auto& player : m_aPlayers)
    {
        player()->Update(delta);
        switch(player()->GetState())
        {
            case Hero::State::REQUEST_RESPAWN:
            {
                auto grave = std::find_if(m_aConstructions.begin(),
                                          m_aConstructions.end(),
                                          [](Construction constr)
                                          {
                                              return constr.eType == Construction::Type::GRAVEYARD;
                                          });
                
                player()->Respawn(grave->stPosition);
                
                auto gs_resp = CreateSVRespawnPlayer(m_oBuilder,
                                                     player.GetUID(),
                                                     player()->GetPosition().x,
                                                     player()->GetPosition().y,
                                                     player()->GetHealth(),
                                                     player()->GetMHealth());
                auto gs_event = CreateMessage(m_oBuilder,
                                            Events_SVRespawnPlayer,
                                            gs_resp.Union());
                m_oBuilder.Finish(gs_event);
                PushEventAndClear();
                
                break;
            }
        }
    }
}

void
GameWorld::ApplyInputEvents()
{
    while(!m_aInEvents.empty()) // received packets are already validated
    {
        auto event = m_aInEvents.front();
        auto gs_event = GetMessage(event.data());
        
        switch(gs_event->event_type())
        {
            case Events_CLActionMove:
            {
                ProcessMoveEvent(static_cast<const CLActionMove*>(gs_event->event()));
                break;
            }
                
            case Events_CLActionItem:
            {
                ProcessItemEvent(static_cast<const CLActionItem*>(gs_event->event()));
                break;
            }
                
            case Events_CLActionSwamp:
            {
                ProcessSwampEvent(static_cast<const CLActionSwamp*>(gs_event->event()));
                break;
            }
                
            case Events_CLActionDuel:
            {
                ProcessDuelEvent(static_cast<const CLActionDuel*>(gs_event->event()));
                break;
            }
                
            case Events_CLActionSpell:
            {
                ProcessSpellEvent(static_cast<const CLActionSpell*>(gs_event->event()));
                break;
            }
                
            case Events_CLActionMap:
            {
                ProcessMapEvent(static_cast<const CLActionMap*>(gs_event->event()));
                break;
            }
                
            default:
                assert(false);
                break;
        }
        
        m_aInEvents.pop(); // remove packet
    }
}

void
GameWorld::UpdateWorld(std::chrono::milliseconds ms)
{
    m_nMonstersTimer += ms.count();
    if(m_nMonstersTimer > 15000)
    {
        m_nMonstersTimer = 0;
        auto pos = GetRandomPosition();
        Monster monster = m_oMonsterFactory.createMonster();
        monster.stPosition = pos;
        m_aMonsters.push_back(monster);
        
        auto gs_monster = CreateSVSpawnMonster(m_oBuilder,
                                               monster.nUID,
                                               monster.stPosition.x,
                                               monster.stPosition.y,
                                               monster.nHP,
                                               monster.nMaxHP);
        
        auto gs_event = CreateMessage(m_oBuilder,
                                    Events_SVSpawnMonster,
                                    gs_monster.Union());
        m_oBuilder.Finish(gs_event);
        PushEventAndClear();
    }
}

void
GameWorld::ProcessSpellEvent(const CLActionSpell* cl_spell)
{
    auto& player = m_aPlayers[FindPlayerByUID(cl_spell->player_uid())];
    player()->SpellCast1();
    
    auto sv_spell = CreateSVActionSpell(m_oBuilder,
                                        cl_spell->player_uid(),
                                        cl_spell->spell_id(),
                                        cl_spell->spell_target(),
                                        cl_spell->x_or_id(),
                                        cl_spell->y());
    auto sv_event = CreateMessage(m_oBuilder,
                                Events_SVActionSpell,
                                sv_spell.Union());
    m_oBuilder.Finish(sv_event);
    PushEventAndClear();
}

void
GameWorld::ProcessMapEvent(const CLActionMap* cl_map)
{
    if(cl_map->act_type() == ActionMapType_DESTROY_BLOCK)
    {
        m_oGameMap[cl_map->x()][cl_map->y()] = GameMap::MapBlockType::NOBLOCK;
    }
    else
    {
        m_oGameMap[cl_map->x()][cl_map->y()] = GameMap::MapBlockType::WALL;
    }
    
    auto sv_map = CreateSVActionMap(m_oBuilder,
                                    cl_map->player_uid(),
                                    cl_map->act_type(),
                                    cl_map->x(),
                                    cl_map->y());
    auto sv_event = CreateMessage(m_oBuilder,
                                Events_SVActionMap,
                                sv_map.Union());
    m_oBuilder.Finish(sv_event);
    PushEventAndClear();
}
