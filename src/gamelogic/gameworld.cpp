//
//  gameworld.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "gameworld.hpp"

#include "../player.hpp"
#include "item.hpp"
#include "construction.hpp"
#include "units/monster.hpp"

#include <chrono>
using namespace std::chrono_literals;

using namespace GameEvent;

GameWorld::GameWorld() :
m_nObjUIDSeq(1),
m_nMonsterSpawnInterval(30s),
m_nMonsterSpawnTimer(30s)
{
    
}

GameWorld::~GameWorld()
{
    for(auto obj : m_apoObjects)
    {
        delete obj;
    }
}

void
GameWorld::SetLoggingSystem(LogSystem * sys)
{
    m_pLogSystem = sys;
}

void
GameWorld::CreateGameMap(const GameMap::Configuration& _conf)
{
    m_stMapConf = _conf;
    
    GameMap gamemap;
    gamemap.GenerateMap(m_stMapConf, this);
    m_nObjUIDSeq = m_apoObjects.back()->GetUID();
    ++m_nObjUIDSeq;
}

std::queue<std::vector<uint8_t>>&
GameWorld::GetIncomingEvents()
{
    return m_aInEvents;
}

std::queue<std::vector<uint8_t>>&
GameWorld::GetOutgoingEvents()
{
    return m_aOutEvents;
}

void
GameWorld::AddPlayer(Player player)
{
    switch(player.GetHeroPicked())
    {
        case Hero::Type::ROGUE:
        {
            auto rogue = new Rogue();
            rogue->SetGameWorld(this);
            rogue->SetUID(player.GetUID());
            rogue->SetName(player.GetNickname());
            m_apoObjects.push_back(rogue);
            break;
        }
        case Hero::Type::WARRIOR:
        {
            auto warrior = new Warrior();
            warrior->SetGameWorld(this);
            warrior->SetUID(player.GetUID());
            warrior->SetName(player.GetNickname());
            m_apoObjects.push_back(warrior);
            break;
        }
        case Hero::Type::MAGE:
        {
            auto mage = new Mage();
            mage->SetGameWorld(this);
            mage->SetUID(player.GetUID());
            mage->SetName(player.GetNickname());
            m_apoObjects.push_back(mage);
            break;
        }
        case Hero::Type::PRIEST:
        {
            auto priest = new Priest();
            priest->SetGameWorld(this);
            priest->SetUID(player.GetUID());
            priest->SetName(player.GetNickname());
            m_apoObjects.push_back(priest);
            break;
        }
        default:
            assert(false);
            break;
    }
}

void
GameWorld::InitialSpawn()
{
    for(auto object : m_apoObjects)
    {
            // spawn players first
        if(object->GetObjType() == GameObject::Type::UNIT)
        {
            auto unit = static_cast<Unit*>(object);
            unit->Spawn(GetRandomPosition());
        }
    }
    
        // spawn key
    auto key = new Key();
    {
        key->SetLogicalPosition(GetRandomPosition());
        key->SetUID(m_nObjUIDSeq);
        m_apoObjects.push_back(key);
        ++m_nObjUIDSeq;
        
            // Log key spawn event
        m_oLogBuilder << "Key spawned at (" << key->GetLogicalPosition().x
        << ";" << key->GetLogicalPosition().y << ")";
        m_pLogSystem->Info(m_oLogBuilder.str());
        m_oLogBuilder.str("");
        
        auto key_spawn = CreateSVSpawnItem(m_oFBuilder,
                                           key->GetUID(),
                                           ItemType_KEY,
                                           key->GetLogicalPosition().x,
                                           key->GetLogicalPosition().y);
        auto msg = CreateMessage(m_oFBuilder,
                                 Events_SVSpawnItem,
                                 key_spawn.Union());
        m_oFBuilder.Finish(msg);
        m_aOutEvents.emplace(m_oFBuilder.GetCurrentBufferPointer(),
                             m_oFBuilder.GetBufferPointer() + m_oFBuilder.GetSize());
        m_oFBuilder.Clear();
    }
    
        // spawn door
    {
        auto door = new Door();
        door->SetLogicalPosition(GetRandomPosition());
        door->SetUID(m_nObjUIDSeq);
        m_apoObjects.push_back(door);
        ++m_nObjUIDSeq;
        
            // Log key spawn event
        m_oLogBuilder << "Door spawned at (" << door->GetLogicalPosition().x
        << ";" << door->GetLogicalPosition().y << ")";
        m_pLogSystem->Info(m_oLogBuilder.str());
        m_oLogBuilder.str("");
        
        auto door_spawn = CreateSVSpawnConstr(m_oFBuilder,
                                              door->GetUID(),
                                              ConstrType_DOOR,
                                              door->GetLogicalPosition().x,
                                              door->GetLogicalPosition().y);
        auto msg = CreateMessage(m_oFBuilder,
                                 Events_SVSpawnConstr,
                                 door_spawn.Union());
        m_oFBuilder.Finish(msg);
        m_aOutEvents.emplace(m_oFBuilder.GetCurrentBufferPointer(),
                             m_oFBuilder.GetBufferPointer() + m_oFBuilder.GetSize());
        m_oFBuilder.Clear();
    }
    
        // spawn graveyard
    {
        auto grave = new Graveyard();
        grave->SetLogicalPosition(GetRandomPosition());
        grave->SetUID(m_nObjUIDSeq);
        m_apoObjects.push_back(grave);
        ++m_nObjUIDSeq;
        
            // Log key spawn event
        m_oLogBuilder << "Graveyard spawned at (" << grave->GetLogicalPosition().x
        << ";" << grave->GetLogicalPosition().y << ")";
        m_pLogSystem->Info(m_oLogBuilder.str());
        m_oLogBuilder.str("");
        
        auto grave_spawn = CreateSVSpawnConstr(m_oFBuilder,
                                               grave->GetUID(),
                                               ConstrType_GRAVEYARD,
                                               grave->GetLogicalPosition().x,
                                               grave->GetLogicalPosition().y);
        auto msg = CreateMessage(m_oFBuilder,
                                 Events_SVSpawnConstr,
                                 grave_spawn.Union());
        m_oFBuilder.Finish(msg);
        m_aOutEvents.emplace(m_oFBuilder.GetCurrentBufferPointer(),
                             m_oFBuilder.GetBufferPointer() + m_oFBuilder.GetSize());
        m_oFBuilder.Clear();
    }
    
    {
            // spawn monster
        auto monster = new Monster();
        monster->SetUID(m_nObjUIDSeq);
        monster->SetGameWorld(this);
        m_apoObjects.push_back(monster);
        monster->Spawn(GetRandomPosition());
        
            // Log monster spawn event
        m_oLogBuilder << "Monster spawned at (" << monster->GetLogicalPosition().x
        << ";" << monster->GetLogicalPosition().y << ")";
        m_pLogSystem->Info(m_oLogBuilder.str());
        m_oLogBuilder.str("");
        
        ++m_nObjUIDSeq; // dont forget!
    }
}

void
GameWorld::ApplyInputEvents()
{
    while(!m_aInEvents.empty())
    {
        auto& event = m_aInEvents.front();
        auto gs_event = GameEvent::GetMessage(event.data());

        switch(gs_event->event_type())
        {
            case GameEvent::Events_CLActionMove:
            {
                auto cl_mov = static_cast<const GameEvent::CLActionMove*>(gs_event->event());
                
                for(auto object : m_apoObjects)
                {
                    if(object->GetObjType() == GameObject::Type::UNIT)
                    {
                        auto unit = dynamic_cast<Unit*>(object);

                        if(unit->GetUID() == cl_mov->target_uid())
                        {
                            unit->Move((Unit::MoveDirection)cl_mov->mov_dir());
                            break;
                        }
                    }
                }
                break;
            }

            case GameEvent::Events_CLActionItem:
            {
                auto gs_item = static_cast<const GameEvent::CLActionItem*>(gs_event->event());
                Item * item = nullptr;
                Hero * player = nullptr;

                auto key_id = gs_item->item_uid();
                for(auto object : m_apoObjects)
                {
                    if(object->GetUID() == gs_item->item_uid())
                    {
                        item = static_cast<Item*>(object);
                    }
                    else if(object->GetUID() == gs_item->player_uid())
                    {
                        player = static_cast<Hero*>(object);
                    }
                }

                switch(gs_item->act_type())
                {
                    case GameEvent::ActionItemType_TAKE:
                    {
                        player->TakeItem(item);
                        break;
                    }

                    case GameEvent::ActionItemType_DROP:
                    {
                        item->SetLogicalPosition(player->GetLogicalPosition());

                            // delete item from players inventory
                        for(auto it = player->GetInventory().begin();
                            it != player->GetInventory().end();
                            ++it)
                        {
                            if((*it)->GetUID() == item->GetUID())
                            {
                                player->GetInventory().erase(it);
                                break;
                            }
                        }
                        player->UpdateStats();

                        break;
                    }

                    default:
                        assert(false);
                        break;
                }

                break;
            }

            case GameEvent::Events_CLActionDuel:
            {
                auto sv_duel = static_cast<const GameEvent::CLActionDuel*>(gs_event->event());

                switch(sv_duel->act_type())
                {
                    case GameEvent::ActionDuelType_STARTED:
                    {
                        Unit * first = nullptr;
                        Unit * second = nullptr;

                        for(auto object : m_apoObjects)
                        {
                            if(object->GetUID() == sv_duel->target1_uid())
                            {
                                first = dynamic_cast<Unit*>(object);
                            }
                            else if(object->GetUID() == sv_duel->target2_uid())
                            {
                                second = dynamic_cast<Unit*>(object);
                            }
                        }
                        
                        if(first->GetState() == Unit::State::WALKING &&
                           second->GetState() == Unit::State::WALKING &&
                           first->GetUnitAttributes() & second->GetUnitAttributes() & Unit::Attributes::DUELABLE &&
                           Distance(first->GetLogicalPosition(), second->GetLogicalPosition()) <= 1.0)
                        {
                            first->StartDuel(second);
                            second->StartDuel(first);
                        }
                        break;
                    }
                    default:
                        assert(false);
                        break;
                }
                break;
            }
            
            case GameEvent::Events_CLActionSpell:
            {
                auto gs_spell = static_cast<const GameEvent::CLActionSpell*>(gs_event->event());
                
                Hero * player = nullptr;
                for(auto object : m_apoObjects)
                {
                    if(object->GetUID() == gs_spell->player_uid())
                    {
                        player = static_cast<Hero*>(object);
                        break;
                    }
                }
                
                player->SpellCast(gs_spell);
                break;
            }
                
            case GameEvent::Events_CLRequestWin:
            {
                auto gs_win = static_cast<const GameEvent::CLRequestWin*>(gs_event->event());
                
                Hero * player = nullptr;
                for(auto obj : m_apoObjects)
                {
                    if(obj->GetUID() == gs_win->player_uid())
                    {
                        player = static_cast<Hero*>(obj);
                        break;
                    }
                }
                
                bool has_key = false;
                for(auto& item : player->GetInventory())
                {
                    if(item->GetType() == Item::Type::KEY)
                    {
                        has_key = true;
                        break;
                    }
                }
                
                bool at_the_door = false;
                for(auto obj : m_apoObjects)
                {
                    if(obj->GetObjType() == GameObject::Type::CONSTRUCTION &&
                       static_cast<Construction*>(obj)->GetType() == Construction::Type::DOOR)
                    {
                        if(Distance(player->GetLogicalPosition(), obj->GetLogicalPosition()) <= 1.0)
                        {
                            at_the_door = true;
                            break;
                        }
                    }
                }
                
                if(has_key && at_the_door)
                {
                        // GAME ENDS
                    auto game_end = CreateSVGameEnd(m_oFBuilder,
                                                    player->GetUID());
                    auto msg = CreateMessage(m_oFBuilder,
                                             Events_SVGameEnd,
                                             game_end.Union());
                    m_oFBuilder.Finish(msg);
                    m_aOutEvents.emplace(m_oFBuilder.GetCurrentBufferPointer(),
                                         m_oFBuilder.GetBufferPointer() + m_oFBuilder.GetSize());
                    m_oFBuilder.Clear();
                    
                        // Log key spawn event
                    m_oLogBuilder << "Player with name '" << player->GetName()
                    << "' won the escaped from LABYRINTH!";
                    m_pLogSystem->Info(m_oLogBuilder.str());
                    m_oLogBuilder.str("");
                }
                
                break;
            }
                
            default:
                assert(false);
                break;
        }
        
        m_aInEvents.pop();
    }
}

void
GameWorld::update(std::chrono::microseconds delta)
{
    ApplyInputEvents();
    for(auto object : m_apoObjects)
    {
        object->update(delta);
    }
        // respawn deads
    Graveyard * grave = nullptr;
    for(auto& obj : m_apoObjects)
    {
        if(obj->GetObjType() == GameObject::Type::CONSTRUCTION &&
           dynamic_cast<Construction*>(obj)->GetType() == Construction::GRAVEYARD)
        {
            grave = dynamic_cast<Graveyard*>(obj);
        }
    }
    
    m_aRespawnQueue.erase(
                         std::remove_if(m_aRespawnQueue.begin(),
                                        m_aRespawnQueue.end(),
                                        [this, delta, grave](auto& unit)
                                        {
                                            if(unit.first <= 0s)
                                            {
                                                unit.second->Respawn(grave->GetLogicalPosition());
                                                return true;
                                            }
                                            else
                                            {
                                                unit.first -= delta;
                                                return false;
                                            }
                                        }),
                         m_aRespawnQueue.end()
                         );
    
        // update monster spawning timer
    m_nMonsterSpawnTimer -= delta;
    if(m_nMonsterSpawnTimer <= 0s)
    {
            // spawn monster
        auto monster = new Monster();
        monster->SetUID(m_nObjUIDSeq);
        monster->SetGameWorld(this);
        m_apoObjects.push_back(monster);
        monster->Spawn(GetRandomPosition());
        
            // Log monster spawn event
        m_oLogBuilder << "Monster spawned at (" << monster->GetLogicalPosition().x
        << ";" << monster->GetLogicalPosition().y << ")";
        m_pLogSystem->Info(m_oLogBuilder.str());
        m_oLogBuilder.str("");
        
        ++m_nObjUIDSeq; // dont forget!
        
        m_nMonsterSpawnTimer = m_nMonsterSpawnInterval;
    }
}

Point2
GameWorld::GetRandomPosition()
{
    Point2 point;
    
    bool point_found = false;
    do
    {
        point.x = m_oRandDistr(m_oRandGen) % (m_stMapConf.nMapSize * m_stMapConf.nRoomSize + 1);
        point.y = m_oRandDistr(m_oRandGen) % (m_stMapConf.nMapSize * m_stMapConf.nRoomSize + 1);
        point_found = true;
        
        for(auto object : m_apoObjects)
        {
            if(object->GetLogicalPosition() == point)
            {
                if(object->GetObjType() != GameObject::Type::MAPBLOCK ||
                   !(object->GetAttributes() & GameObject::Attributes::PASSABLE))
                {
                    point_found = false;
                    break;
                }
            }
        }
    } while(!point_found);
    
    return point;
}
