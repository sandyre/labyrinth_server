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
#include "mapblock.hpp"
#include "units/monster.hpp"

#include <chrono>
using namespace std::chrono_literals;
using namespace GameMessage;


GameWorld::GameWorld(const GameMapGenerator::Configuration& conf,
                     std::vector<PlayerInfo>& players)
: _monsterSpawnInterval(30s),
  _monsterSpawnTimer(30s),
  _mapConf(conf),
  _logger("World", NamedLogger::Mode::STDIO)
{
    auto map = GameMapGenerator::GenerateMap(conf);

        // create floor
    for(int i = map.size()-1; i >= 0; --i)
    {
        for(int j = map.size()-1; j >= 0; --j)
        {
            auto block = _objectFactory.Create<NoBlock>(*this);
            block->SetPosition(Point<>(i, j));
            _objects.push_back(block);
        }
    }

    for(int i = map.size()-1; i >= 0; --i)
    {
        for(int j = map.size()-1; j >= 0; --j)
        {
            if(map[i][j] == GameMapGenerator::MapBlockType::WALL)
            {
                auto block = _objectFactory.Create<WallBlock>(*this);
                block->SetPosition(Point<>(i, j));
                _objects.push_back(block);
            }
            else if(map[i][j] == GameMapGenerator::MapBlockType::BORDER)
            {
                auto block = _objectFactory.Create<BorderBlock>(*this);
                block->SetPosition(Point<>(i, j));
                _objects.push_back(block);
            }
        }
    }

    for(auto& player : players)
    {
    switch(player.Hero)
    {
    case Hero::Type::ROGUE:
    {
        auto rogue = _objectFactory.Create<Rogue>(*this, player.LocalUid);
        rogue->SetName(player.Name);
        _objects.push_back(rogue);
        break;
    }
    case Hero::Type::WARRIOR:
    {
        auto warrior = _objectFactory.Create<Warrior>(*this, player.LocalUid);
        warrior->SetName(player.Name);
        _objects.push_back(warrior);
        break;
    }
    case Hero::Type::MAGE:
    {
        auto mage = _objectFactory.Create<Mage>(*this, player.LocalUid);
        mage->SetName(player.Name);
        _objects.push_back(mage);
        break;
    }
    case Hero::Type::PRIEST:
    {
        auto priest = _objectFactory.Create<Priest>(*this, player.LocalUid);
        priest->SetName(player.Name);
        _objects.push_back(priest);
        break;
    }
    default:
        assert(false);
        break;
    }
    }

    InitialSpawn();
}


void
GameWorld::InitialSpawn()
{
    for(auto& object : _objects)
    {
        if(object->GetType() == GameObject::Type::UNIT)
        {
            auto unit = std::dynamic_pointer_cast<Unit>(object);
            unit->Spawn(GetRandomPosition());
        }
    }
    
        // spawn key
    auto key = _objectFactory.Create<Key>(*this);
    {
        key->SetPosition(GetRandomPosition());
        _objects.push_back(key);
        
            // Log key spawn event
        _logger.Info() << "Key spawned at (" << key->GetPosition().x << "," << key->GetPosition().y << ")";

        auto key_spawn = CreateSVSpawnItem(_flatBuilder,
                                           key->GetUID(),
                                           ItemType_KEY,
                                           key->GetPosition().x,
                                           key->GetPosition().y);
        auto msg = CreateMessage(_flatBuilder,
                                 0,
                                 Messages_SVSpawnItem,
                                 key_spawn.Union());
        _flatBuilder.Finish(msg);
        _outputEvents.emplace(_flatBuilder.GetCurrentBufferPointer(),
                             _flatBuilder.GetBufferPointer() + _flatBuilder.GetSize());
        _flatBuilder.Clear();
    }
    
        // spawn door
    {
        auto door = _objectFactory.Create<Door>(*this);
        door->SetPosition(GetRandomPosition());
        _objects.push_back(door);
        
            // Log key spawn event
        _logger.Info() << "Door spawned at (" << door->GetPosition().x << "," << door->GetPosition().y << ")";
        
        auto door_spawn = CreateSVSpawnConstr(_flatBuilder,
                                              door->GetUID(),
                                              ConstrType_DOOR,
                                              door->GetPosition().x,
                                              door->GetPosition().y);
        auto msg = CreateMessage(_flatBuilder,
                                 0,
                                 Messages_SVSpawnConstr,
                                 door_spawn.Union());
        _flatBuilder.Finish(msg);
        _outputEvents.emplace(_flatBuilder.GetCurrentBufferPointer(),
                             _flatBuilder.GetBufferPointer() + _flatBuilder.GetSize());
        _flatBuilder.Clear();
    }
    
        // spawn graveyard
    {
        auto grave = _objectFactory.Create<Graveyard>(*this);
        grave->SetPosition(GetRandomPosition());
        _objects.push_back(grave);
        
            // Log key spawn event
        _logger.Info() << "Graveyard spawned at (" << grave->GetPosition().x << "," << grave->GetPosition().y << ")";
        
        auto grave_spawn = CreateSVSpawnConstr(_flatBuilder,
                                               grave->GetUID(),
                                               ConstrType_GRAVEYARD,
                                               grave->GetPosition().x,
                                               grave->GetPosition().y);
        auto msg = CreateMessage(_flatBuilder,
                                 0,
                                 Messages_SVSpawnConstr,
                                 grave_spawn.Union());
        _flatBuilder.Finish(msg);
        _outputEvents.emplace(_flatBuilder.GetCurrentBufferPointer(),
                             _flatBuilder.GetBufferPointer() + _flatBuilder.GetSize());
        _flatBuilder.Clear();
    }
    
    {
            // spawn monster
        auto monster = _objectFactory.Create<Monster>(*this);
        _objects.push_back(monster);
        monster->Spawn(GetRandomPosition());
        
            // Log monster spawn event
        _logger.Info() << "Monster spawned at (" << monster->GetPosition().x << "," << monster->GetPosition().y << ")";
    }
}

void
GameWorld::ApplyInputEvents()
{
    while(!_inputEvents.empty())
    {
        auto& event = _inputEvents.front();
        auto gs_event = GameMessage::GetMessage(event.data());

        switch(gs_event->payload_type())
        {
        case GameMessage::Messages_CLActionMove:
        {
            auto cl_mov = static_cast<const GameMessage::CLActionMove*>(gs_event->payload());

            auto obj = std::find_if(_objects.begin(),
                                    _objects.end(),
                                    [cl_mov](const std::shared_ptr<GameObject>& obj)
                                    {
                                        return obj->GetUID() == cl_mov->target_uid();
                                    });

            if(obj == _objects.end())
            {
                _logger.Warning() << "Received CLMove with unknown target_uid";
                continue;
            }

            auto unit = std::dynamic_pointer_cast<Unit>(*obj);
            unit->Move((Unit::MoveDirection)cl_mov->mov_dir());

            break;
        }

        case GameMessage::Messages_CLActionItem:
        {
            auto cl_item = static_cast<const GameMessage::CLActionItem*>(gs_event->payload());

            switch(cl_item->act_type())
            {
            case ActionItemType_TAKE:
            {
                auto item_obj = std::find_if(_objects.begin(),
                                             _objects.end(),
                                             [cl_item](const std::shared_ptr<GameObject>& obj)
                                             {
                                                 return obj->GetUID() == cl_item->item_uid();
                                             });
                auto unit_obj = std::find_if(_objects.begin(),
                                             _objects.end(),
                                             [cl_item](const std::shared_ptr<GameObject>& obj)
                                             {
                                                 return obj->GetUID() == cl_item->player_uid();
                                             });

                if(item_obj == _objects.end() ||
                   unit_obj == _objects.end())
                {
                    _logger.Warning() << "Received CLItem::TAKE with invalid item_uid and/or player_uid";
                    continue;
                }

                auto item = std::dynamic_pointer_cast<Item>(*item_obj);
                auto player = std::dynamic_pointer_cast<Unit>(*unit_obj);

                player->TakeItem(item);
                _objects.erase(item_obj);

                break;
            }

            case ActionItemType_DROP:
            {
                auto unit_obj = std::find_if(_objects.begin(),
                                             _objects.end(),
                                             [cl_item](const std::shared_ptr<GameObject>& obj)
                                             {
                                                 return obj->GetUID() == cl_item->player_uid();
                                             });

                if(unit_obj == _objects.end())
                {
                    _logger.Warning() << "Received CLItem::DROP with invalid target_uid";
                    continue;
                }

                auto player = std::dynamic_pointer_cast<Unit>(*unit_obj);
                auto item = player->DropItem(cl_item->item_uid());
                if(item)
                    _objects.push_back(item);

                break;
            }

            default:
                _logger.Warning() << "Received unhandled CLItem packet type";
                break;
            }

            break;
        }

        case GameMessage::Messages_CLActionDuel:
        {
            auto sv_duel = static_cast<const GameMessage::CLActionDuel*>(gs_event->payload());

            switch(sv_duel->act_type())
            {
            case GameMessage::ActionDuelType_STARTED:
            {
                std::shared_ptr<Unit> first, second;

                for(auto object : _objects)
                {
                    if(object->GetUID() == sv_duel->target1_uid())
                        first = std::dynamic_pointer_cast<Unit>(object);
                    else if(object->GetUID() == sv_duel->target2_uid())
                        second = std::dynamic_pointer_cast<Unit>(object);
                }
                
                if(first->GetState() == Unit::State::WALKING &&
                   second->GetState() == Unit::State::WALKING &&
                   first->GetUnitAttributes() & second->GetUnitAttributes() & Unit::Attributes::DUELABLE &&
                   first->GetPosition().Distance(second->GetPosition()) <= 1.0)
                {
                    first->StartDuel(second);
                    second->StartDuel(first);
                }
                break;
            }
            default:
                _logger.Warning() << "Received unhandled CLDuel event type";
                break;
            }
            break;
        }
        
        case GameMessage::Messages_CLActionSpell:
        {
            auto cl_spell = static_cast<const GameMessage::CLActionSpell*>(gs_event->payload());

            auto unit_obj = std::find_if(_objects.begin(),
                                         _objects.end(),
                                         [cl_spell](const std::shared_ptr<GameObject>& obj)
                                         {
                                             return obj->GetUID() == cl_spell->player_uid();
                                         });

            if(unit_obj == _objects.end())
            {
                _logger.Warning() << "Received CLSpell event with unknown player_uid";
                continue;
            }

            std::shared_ptr<Unit> unit = std::dynamic_pointer_cast<Unit>(*unit_obj);
            
            unit->SpellCast(cl_spell);
            break;
        }
            
        case GameMessage::Messages_CLRequestWin:
        {
            auto cl_win = static_cast<const GameMessage::CLRequestWin*>(gs_event->payload());

            auto unit_obj = std::find_if(_objects.begin(),
                                         _objects.end(),
                                         [cl_win](const std::shared_ptr<GameObject>& obj)
                                         {
                                             return obj->GetUID() == cl_win->player_uid();
                                         });

            if(unit_obj == _objects.end())
            {
                _logger.Warning() << "Received CLRequestWin event with unknown player_uid";
                continue;
            }

            auto player = std::dynamic_pointer_cast<Unit>(*unit_obj);
            auto& inventory = player->GetInventory();
            bool has_key = std::any_of(inventory.begin(),
                                       inventory.end(),
                                       [](const std::shared_ptr<Item>& item)
                                       {
                                           return item->GetType() == Item::Type::KEY;
                                       });

            auto door_obj = std::find_if(_objects.begin(),
                                         _objects.end(),
                                         [](const std::shared_ptr<GameObject>& obj)
                                         {
                                             if(obj->GetType() == GameObject::Type::CONSTRUCTION &&
                                                std::dynamic_pointer_cast<Construction>(obj)->GetType() == Construction::Type::DOOR)
                                             {
                                                 return true;
                                             }
                                             return false;
                                         });
            if(door_obj != _objects.end())
                throw std::runtime_error("Door is not presented in gameworld!!!");

            if(has_key && player->GetPosition() == (*door_obj)->GetPosition())
            {
                    // GAME ENDS
                auto game_end = CreateSVGameEnd(_flatBuilder,
                                                player->GetUID());
                auto msg = CreateMessage(_flatBuilder,
                                         0,
                                         Messages_SVGameEnd,
                                         game_end.Union());
                _flatBuilder.Finish(msg);
                _outputEvents.emplace(_flatBuilder.GetCurrentBufferPointer(),
                                      _flatBuilder.GetBufferPointer() + _flatBuilder.GetSize());
                _flatBuilder.Clear();

                _logger.Info() << "Player with name '" << player->GetName() << "' won! Escaped from LABYRINTH!";
            }
            
            break;
        }
            
        default:
            _logger.Warning() << "Received undefined packet type";
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
    std::shared_ptr<Graveyard> grave = nullptr;
    for(auto& obj : _objects)
    {
        if(obj->GetType() == GameObject::Type::CONSTRUCTION &&
           std::dynamic_pointer_cast<Construction>(obj)->GetType() == Construction::GRAVEYARD)
        {
            grave = std::dynamic_pointer_cast<Graveyard>(obj);
        }
    }
    
    _respawnQueue.erase(
                         std::remove_if(_respawnQueue.begin(),
                                        _respawnQueue.end(),
                                        [this, delta, grave](auto& unit)
                                        {
                                            if(unit.first <= 0s)
                                            {
                                                unit.second->Respawn(grave->GetPosition());
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
        auto monster = _objectFactory.Create<Monster>(*this);
        _objects.push_back(monster);
        
        monster->Spawn(GetRandomPosition());

        _monsterSpawnTimer = _monsterSpawnInterval;
    }
}


Point<>
GameWorld::GetRandomPosition()
{
    Point<> point;
    
    bool point_found = false;
    do
    {
        point.x = _randDistr(_randGenerator) % (_mapConf.MapSize * _mapConf.RoomSize + 1);
        point.y = _randDistr(_randGenerator) % (_mapConf.MapSize * _mapConf.RoomSize + 1);
        point_found = true;
        
        for(auto object : _objects)
        {
            if(object->GetPosition() == point)
            {
                if(object->GetType() != GameObject::Type::MAPBLOCK ||
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
