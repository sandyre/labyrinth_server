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
#include <list>
using namespace std::chrono_literals;
using namespace GameMessage;


GameWorld::GameWorld(const GameMapGenerator::Configuration& conf,
                     std::vector<PlayerInfo>& players)
: _monsterSpawnInterval(30s),
  _monsterSpawnTimer(30s),
  _mapConf(conf),
  _objectsStorage(*this),
  _logger("World", NamedLogger::Mode::STDIO)
{
    auto map = GameMapGenerator::GenerateMap(conf);

        // create floor
    for(int i = map.size()-1; i >= 0; --i)
    {
        for(int j = map.size()-1; j >= 0; --j)
        {
            auto block = _objectsStorage.Create<NoBlock>();
            block->SetPosition(Point<>(i, j));
        }
    }

    for(int i = map.size()-1; i >= 0; --i)
    {
        for(int j = map.size()-1; j >= 0; --j)
        {
            if(map[i][j] == GameMapGenerator::MapBlockType::WALL)
            {
                auto block = _objectsStorage.Create<WallBlock>();
                block->SetPosition(Point<>(i, j));
            }
            else if(map[i][j] == GameMapGenerator::MapBlockType::BORDER)
            {
                auto block = _objectsStorage.Create<BorderBlock>();
                block->SetPosition(Point<>(i, j));
            }
        }
    }

    for(auto& player : players)
    {
    switch(player.Hero)
    {
    case Hero::Type::WARRIOR:
    {
        auto warrior = _objectsStorage.Create<Warrior>(player.LocalUid);
        warrior->SetName(player.Name);
        break;
    }
    case Hero::Type::MAGE:
    {
        auto mage = _objectsStorage.Create<Mage>(player.LocalUid);
        mage->SetName(player.Name);
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
    auto units = _objectsStorage.Subset<Unit>();
    for(auto& unit : units)
        unit->Spawn(GetRandomPosition());
    
        // spawn key
    auto key = _objectsStorage.Create<Key>();
    {
        key->SetPosition(GetRandomPosition());
        
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
        auto door = _objectsStorage.Create<Door>();
        door->SetPosition(GetRandomPosition());
        
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
        auto grave = _objectsStorage.Create<Graveyard>();
        grave->SetPosition(GetRandomPosition());

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
        auto monster = _objectsStorage.Create<Monster>();
        monster->Spawn(GetRandomPosition());
        
            // Log monster spawn event
        _logger.Info() << "Monster spawned at (" << monster->GetPosition().x << "," << monster->GetPosition().y << ")";
    }

    _logger.Info() << "Initial spawn done, total number of GameObjects: " << _objectsStorage.Size();
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

            if(auto unit = _objectsStorage.FindObject<Unit>(cl_mov->target_uid()))
                unit->Move((Unit::MoveDirection)cl_mov->mov_dir());
            else
                _logger.Warning() << "Received CLMove with unknown target_uid";

            break;
        }

        case GameMessage::Messages_CLActionItem:
        {
            auto cl_item = static_cast<const GameMessage::CLActionItem*>(gs_event->payload());

            switch(cl_item->act_type())
            {
            case ActionItemType_TAKE:
            {
                auto item = _objectsStorage.FindObject<Item>(cl_item->item_uid());
                auto unit = _objectsStorage.FindObject<Unit>(cl_item->player_uid());

                if(item && unit)
                {
                    unit->TakeItem(item);
                    _objectsStorage.DeleteObject(item);
                }
                else
                    _logger.Warning() << "Received CLItem::TAKE with invalid item_uid and/or player_uid";

                break;
            }

            case ActionItemType_DROP:
            {
                auto unit = _objectsStorage.FindObject<Unit>(cl_item->player_uid());

                if(unit)
                {
                    auto item = unit->DropItem(cl_item->item_uid());
                    if(item)
                        _objectsStorage.PushObject(item);
                    else
                        _logger.Warning() << "Received CLItem::DROP with item_uid, that player cant drop";
                }
                else
                    _logger.Warning() << "Received CLItem::DROP with invalid target_uid";

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
            auto cl_duel = static_cast<const GameMessage::CLActionDuel*>(gs_event->payload());

            switch(cl_duel->act_type())
            {
            case GameMessage::ActionDuelType_STARTED:
            {
                auto first = _objectsStorage.FindObject<Unit>(cl_duel->target1_uid());
                auto second = _objectsStorage.FindObject<Unit>(cl_duel->target2_uid());
                
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

            if(auto unit = _objectsStorage.FindObject<Unit>(cl_spell->player_uid()))
                unit->SpellCast(cl_spell);
            else
                _logger.Warning() << "Received CLSpell event with unknown player_uid";

            break;
        }
            
        case GameMessage::Messages_CLRequestWin:
        {
            auto cl_win = static_cast<const GameMessage::CLRequestWin*>(gs_event->payload());

            auto unit = _objectsStorage.FindObject<Unit>(cl_win->player_uid());
            if(!unit)
                _logger.Warning() << "Received CLRequestWin event with unknown player_uid";

            auto& inventory = unit->GetInventory();
            bool has_key = std::any_of(inventory.begin(),
                                       inventory.end(),
                                       [](const std::shared_ptr<Item>& item)
                                       {
                                           return item->GetType() == Item::Type::KEY;
                                       });

            auto doors = _objectsStorage.Subset<Door>(); // FIXME: there is only ONE door. But potentially there are many
            assert(!doors.empty());

            if(has_key && unit->GetPosition() == doors[0]->GetPosition())
            {
                    // GAME ENDS
                auto game_end = CreateSVGameEnd(_flatBuilder,
                                                unit->GetUID());
                auto msg = CreateMessage(_flatBuilder,
                                         0,
                                         Messages_SVGameEnd,
                                         game_end.Union());
                _flatBuilder.Finish(msg);
                _outputEvents.emplace(_flatBuilder.GetCurrentBufferPointer(),
                                      _flatBuilder.GetBufferPointer() + _flatBuilder.GetSize());
                _flatBuilder.Clear();

                _logger.Info() << "Player with name '" << unit->GetName() << "' won! Escaped from LABYRINTH!";
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
    std::for_each(_objectsStorage.Begin(),
                  _objectsStorage.End(),
                  [delta](const GameObjectPtr& obj)
                  {
                      obj->update(delta);
                  });

        // respawn deads
    auto grave = _objectsStorage.Subset<Graveyard>(); // FIXME: one grave, but potentially many
    
    _respawnQueue.erase(
                         std::remove_if(_respawnQueue.begin(),
                                        _respawnQueue.end(),
                                        [this, delta, grave](auto& unit)
                                        {
                                            if(unit.first <= 0s)
                                            {
                                                unit.second->Respawn(grave[0]->GetPosition());
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
        auto monster = _objectsStorage.Create<Monster>();
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

        for(auto iter = _objectsStorage.Begin(); iter != _objectsStorage.End(); ++iter)
        {
            if((*iter)->GetPosition() == point)
            {
                if((*iter)->GetType() != GameObject::Type::MAPBLOCK ||
                   !((*iter)->GetAttributes() & GameObject::Attributes::PASSABLE))
                {
                    point_found = false;
                    break;
                }
            }
        }
    } while(!point_found);
    
    return point;
}
