//
//  gameworld.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "gameworld.hpp"

#include "construction.hpp"
#include "item.hpp"
#include "units/monster.hpp"
#include "../player.hpp"

#include <chrono>
using namespace std::chrono_literals;

using namespace GameEvent;

GameWorld::GameWorld() :
_objUIDSeq(1),
_monsterSpawnInterval(30s),
_monsterSpawnTimer(30s),
_logger("GameWorld", NamedLogger::Mode::STDIO)
{ }

GameWorld::~GameWorld()
{
    for(auto obj : _objects)
    {
        delete obj;
    }
}

void
GameWorld::CreateGameMap(const GameMap::Configuration& _conf)
{
    m_stMapConf = _conf;
    
    GameMap gamemap;
    gamemap.GenerateMap(m_stMapConf, this);
    _objUIDSeq = _objects.back()->GetUID();
    ++_objUIDSeq;
}

std::queue<std::vector<uint8_t>>&
GameWorld::GetIncomingEvents()
{
    return _inputEvents;
}

std::queue<std::vector<uint8_t>>&
GameWorld::GetOutgoingEvents()
{
    return _outputEvents;
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
            _objects.push_back(rogue);
            break;
        }
        case Hero::Type::WARRIOR:
        {
            auto warrior = new Warrior();
            warrior->SetGameWorld(this);
            warrior->SetUID(player.GetUID());
            warrior->SetName(player.GetNickname());
            _objects.push_back(warrior);
            break;
        }
        case Hero::Type::MAGE:
        {
            auto mage = new Mage();
            mage->SetGameWorld(this);
            mage->SetUID(player.GetUID());
            mage->SetName(player.GetNickname());
            _objects.push_back(mage);
            break;
        }
        case Hero::Type::PRIEST:
        {
            auto priest = new Priest();
            priest->SetGameWorld(this);
            priest->SetUID(player.GetUID());
            priest->SetName(player.GetNickname());
            _objects.push_back(priest);
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
    for(auto object : _objects)
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
        key->SetUID(_objUIDSeq);
        _objects.push_back(key);
        ++_objUIDSeq;
        
            // Log key spawn event
        _logger.Info() << "Key spawned at (" << key->GetLogicalPosition().x << "," << key->GetLogicalPosition().y << ")" << End();

        auto key_spawn = CreateSVSpawnItem(_flatBuilder,
                                           key->GetUID(),
                                           ItemType_KEY,
                                           key->GetLogicalPosition().x,
                                           key->GetLogicalPosition().y);
        auto msg = CreateMessage(_flatBuilder,
                                 0,
                                 Events_SVSpawnItem,
                                 key_spawn.Union());
        _flatBuilder.Finish(msg);
        _outputEvents.emplace(_flatBuilder.GetCurrentBufferPointer(),
                             _flatBuilder.GetBufferPointer() + _flatBuilder.GetSize());
        _flatBuilder.Clear();
    }
    
        // spawn door
    {
        auto door = new Door();
        door->SetLogicalPosition(GetRandomPosition());
        door->SetUID(_objUIDSeq);
        _objects.push_back(door);
        ++_objUIDSeq;
        
            // Log key spawn event
        _logger.Info() << "Door spawned at (" << door->GetLogicalPosition().x << "," << door->GetLogicalPosition().y << ")" << End();
        
        auto door_spawn = CreateSVSpawnConstr(_flatBuilder,
                                              door->GetUID(),
                                              ConstrType_DOOR,
                                              door->GetLogicalPosition().x,
                                              door->GetLogicalPosition().y);
        auto msg = CreateMessage(_flatBuilder,
                                 0,
                                 Events_SVSpawnConstr,
                                 door_spawn.Union());
        _flatBuilder.Finish(msg);
        _outputEvents.emplace(_flatBuilder.GetCurrentBufferPointer(),
                             _flatBuilder.GetBufferPointer() + _flatBuilder.GetSize());
        _flatBuilder.Clear();
    }
    
        // spawn graveyard
    {
        auto grave = new Graveyard();
        grave->SetLogicalPosition(GetRandomPosition());
        grave->SetUID(_objUIDSeq);
        _objects.push_back(grave);
        ++_objUIDSeq;
        
            // Log key spawn event
        _logger.Info() << "Graveyard spawned at (" << grave->GetLogicalPosition().x << "," << grave->GetLogicalPosition().y << ")" << End();
        
        auto grave_spawn = CreateSVSpawnConstr(_flatBuilder,
                                               grave->GetUID(),
                                               ConstrType_GRAVEYARD,
                                               grave->GetLogicalPosition().x,
                                               grave->GetLogicalPosition().y);
        auto msg = CreateMessage(_flatBuilder,
                                 0,
                                 Events_SVSpawnConstr,
                                 grave_spawn.Union());
        _flatBuilder.Finish(msg);
        _outputEvents.emplace(_flatBuilder.GetCurrentBufferPointer(),
                             _flatBuilder.GetBufferPointer() + _flatBuilder.GetSize());
        _flatBuilder.Clear();
    }
    
    {
            // spawn monster
        auto monster = new Monster();
        monster->SetUID(_objUIDSeq);
        monster->SetGameWorld(this);
        _objects.push_back(monster);
        monster->Spawn(GetRandomPosition());
        
            // Log monster spawn event
        _logger.Info() << "Monster spawned at (" << monster->GetLogicalPosition().x << "," << monster->GetLogicalPosition().y << ")" << End();
        
        ++_objUIDSeq; // dont forget!
    }
}

void
GameWorld::ApplyInputEvents()
{
    while(!_inputEvents.empty())
    {
        auto& event = _inputEvents.front();
        auto gs_event = GameEvent::GetMessage(event.data());

        switch(gs_event->event_type())
        {
            case GameEvent::Events_CLActionMove:
            {
                auto cl_mov = static_cast<const GameEvent::CLActionMove*>(gs_event->event());
                
                for(auto object : _objects)
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
                for(auto object : _objects)
                {
                    if(object->GetUID() == gs_item->item_uid())
                    {
                        item = static_cast<Item*>(object);
                    }
                    else if(object->GetUID() == gs_event->sender_id())
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

                        for(auto object : _objects)
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
                for(auto object : _objects)
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
                for(auto obj : _objects)
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
                for(auto obj : _objects)
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
                    auto game_end = CreateSVGameEnd(_flatBuilder,
                                                    player->GetUID());
                    auto msg = CreateMessage(_flatBuilder,
                                             0,
                                             Events_SVGameEnd,
                                             game_end.Union());
                    _flatBuilder.Finish(msg);
                    _outputEvents.emplace(_flatBuilder.GetCurrentBufferPointer(),
                                         _flatBuilder.GetBufferPointer() + _flatBuilder.GetSize());
                    _flatBuilder.Clear();
                    
                        // Log key spawn event
                    _logger.Info() << "Player with name '" << player->GetName() << "' won the escaped from LABYRINTH!" << End();
                }
                
                break;
            }
                
            default:
                assert(false);
                break;
        }
        
        _inputEvents.pop();
    }
}

void
GameWorld::update(std::chrono::microseconds delta)
{
    ApplyInputEvents();
    for(auto object : _objects)
    {
        object->update(delta);
    }
        // respawn deads
    Graveyard * grave = nullptr;
    for(auto& obj : _objects)
    {
        if(obj->GetObjType() == GameObject::Type::CONSTRUCTION &&
           dynamic_cast<Construction*>(obj)->GetType() == Construction::GRAVEYARD)
        {
            grave = dynamic_cast<Graveyard*>(obj);
        }
    }
    
    _respawnQueue.erase(
                         std::remove_if(_respawnQueue.begin(),
                                        _respawnQueue.end(),
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
                         _respawnQueue.end()
                         );
    
        // update monster spawning timer
    _monsterSpawnTimer -= delta;
    if(_monsterSpawnTimer <= 0s)
    {
            // spawn monster
        auto monster = new Monster();
        monster->SetUID(_objUIDSeq);
        monster->SetGameWorld(this);
        _objects.push_back(monster);
        monster->Spawn(GetRandomPosition());
        
            // Log monster spawn event
        _logger.Info() << "Monster spawned at (" << monster->GetLogicalPosition().x << "," << monster->GetLogicalPosition().y << ")" << End();
        
        ++_objUIDSeq; // dont forget!
        
        _monsterSpawnTimer = _monsterSpawnInterval;
    }
}

Point2
GameWorld::GetRandomPosition()
{
    Point2 point;
    
    bool point_found = false;
    do
    {
        point.x = _randDistr(_randGenerator) % (m_stMapConf.MapSize * m_stMapConf.RoomSize + 1);
        point.y = _randDistr(_randGenerator) % (m_stMapConf.MapSize * m_stMapConf.RoomSize + 1);
        point_found = true;
        
        for(auto object : _objects)
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
