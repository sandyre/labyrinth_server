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
m_nObjUIDSeq(1)
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
        m_pLogSystem->Write(m_oLogBuilder.str());
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
    
    {
            // spawn monster
        auto monster = new Monster();
        monster->SetUID(m_nObjUIDSeq);
        monster->SetGameWorld(this);
        m_apoObjects.push_back(monster);
        monster->Spawn(GetRandomPosition());
        monster->TakeItem(key);
        
            // Log monster spawn event
        m_oLogBuilder << "Monster spawned at (" << monster->GetLogicalPosition().x
        << ";" << monster->GetLogicalPosition().y << ")";
        m_pLogSystem->Write(m_oLogBuilder.str());
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
                        auto unit = static_cast<Unit*>(object);

                        if(unit->GetUID() == cl_mov->target_uid())
                        {
                            unit->Move(Point2(cl_mov->x(),
                                              cl_mov->y()));
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
                                first = static_cast<Unit*>(object);
                            }
                            else if(object->GetUID() == sv_duel->target2_uid())
                            {
                                second = static_cast<Unit*>(object);
                            }
                        }
                        
                        if(first->GetAttributes() & second->GetAttributes() & GameObject::Attributes::DUELABLE)
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
            case GameEvent::Events_CLActionAttack:
            {
                auto atk = static_cast<const GameEvent::CLActionAttack*>(gs_event->event());
                
                Unit * attacker = nullptr;
                
                for(auto object : m_apoObjects)
                {
                    if(object->GetUID() == atk->target1_uid())
                    {
                        attacker = static_cast<Unit*>(object);
                    }
                }
                
                attacker->Attack(atk);
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
                    }
                }
                
                player->SpellCast(gs_spell);
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
GameWorld::update(std::chrono::milliseconds delta)
{
    ApplyInputEvents();
    for(auto object : m_apoObjects)
    {
        object->update(delta);
    }
        // respawn deads
    m_aRespawnQueue.erase(
                         std::remove_if(m_aRespawnQueue.begin(),
                                        m_aRespawnQueue.end(),
                                        [this, delta](auto& unit)
                                        {
                                            if(unit.first <= 0s)
                                            {
                                                unit.second->Respawn(GetRandomPosition());
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
