//
//  gameworld.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "gameworld.hpp"

#include "item.hpp"
#include "mapblock.hpp"
#include "units/monster.hpp"

#include <chrono>
#include <list>
using namespace std::chrono_literals;
using namespace GameMessage;


GameWorld::GameWorld(const GameMapGenerator::Configuration& conf,
                     std::vector<PlayerInfo>& players)
: _mapConf(conf),
  _state(State::RUNNING),
  _objectsStorage(*this),
  _respawner(*this),
  _monsterSpawner(*this),
  _randGen(0, 1000, 0),
  _logger("World", NamedLogger::Mode::STDIO)
{
    auto map = GameMapGenerator::GenerateMap(conf);

        // create floor
    for(int i = map.size()-1; i >= 0; --i)
    {
        for(int j = map.size()-1; j >= 0; --j)
        {
            auto block = _objectsStorage.Create<NoBlock>();
            block->Spawn(Point<>(i, j));
        }
    }

    for(int i = map.size()-1; i >= 0; --i)
    {
        for(int j = map.size()-1; j >= 0; --j)
        {
            if(map[i][j] == GameMapGenerator::MapBlockType::WALL)
            {
                auto block = _objectsStorage.Create<WallBlock>();
                block->Spawn(Point<>(i, j));
            }
            else if(map[i][j] == GameMapGenerator::MapBlockType::BORDER)
            {
                auto block = _objectsStorage.Create<BorderBlock>();
                block->Spawn(Point<>(i, j));
            }
        }
    }

    for(auto& player : players)
    {
        switch(player.Hero)
        {
        case Hero::Type::WARRIOR:
        {
            auto warrior = _objectsStorage.CreateWithUID<Warrior>(player.LocalUid);
            warrior->SetName(player.Name);
            break;
        }
        case Hero::Type::MAGE:
        {
            auto mage = _objectsStorage.CreateWithUID<Mage>(player.LocalUid);
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
        key->Spawn(GetRandomPosition());
        
            // Log key spawn event
        _logger.Info() << "Key spawned at (" << key->GetPosition().x << "," << key->GetPosition().y << ")";

        flatbuffers::FlatBufferBuilder builder;
        auto key_spawn = CreateSVSpawnItem(builder,
                                           key->GetUID(),
                                           ItemType_KEY,
                                           key->GetPosition().x,
                                           key->GetPosition().y);
        auto msg = CreateMessage(builder,
                                 0,
                                 Messages_SVSpawnItem,
                                 key_spawn.Union());
        builder.Finish(msg);
        _outputEvents.emplace(builder.GetCurrentBufferPointer(),
                              builder.GetBufferPointer() + builder.GetSize());
    }
    
        // spawn door
    {
        auto door = _objectsStorage.Create<Door>();
        door->Spawn(GetRandomPosition());
        
            // Log key spawn event
        _logger.Info() << "Door spawned at (" << door->GetPosition().x << "," << door->GetPosition().y << ")";

        flatbuffers::FlatBufferBuilder builder;
        auto door_spawn = CreateSVSpawnConstr(builder,
                                              door->GetUID(),
                                              ConstrType_DOOR,
                                              door->GetPosition().x,
                                              door->GetPosition().y);
        auto msg = CreateMessage(builder,
                                 0,
                                 Messages_SVSpawnConstr,
                                 door_spawn.Union());
        builder.Finish(msg);
        _outputEvents.emplace(builder.GetCurrentBufferPointer(),
                              builder.GetBufferPointer() + builder.GetSize());
    }
    
        // spawn graveyard
    {
        auto grave = _objectsStorage.Create<Graveyard>();
        grave->Spawn(GetRandomPosition());

            // Log key spawn event
        _logger.Info() << "Graveyard spawned at (" << grave->GetPosition().x << "," << grave->GetPosition().y << ")";

        flatbuffers::FlatBufferBuilder builder;
        auto grave_spawn = CreateSVSpawnConstr(builder,
                                               grave->GetUID(),
                                               ConstrType_GRAVEYARD,
                                               grave->GetPosition().x,
                                               grave->GetPosition().y);
        auto msg = CreateMessage(builder,
                                 0,
                                 Messages_SVSpawnConstr,
                                 grave_spawn.Union());
        builder.Finish(msg);
        _outputEvents.emplace(builder.GetCurrentBufferPointer(),
                              builder.GetBufferPointer() + builder.GetSize());
    }

    _logger.Info() << "Initial spawn done, total number of GameObjects: " << _objectsStorage.Size();
}

void
GameWorld::ApplyInputEvents()
{
    while(!_inputMessages.empty())
    {
        auto& event = _inputMessages.front();
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
                if(auto unit = _objectsStorage.FindObject<Unit>(cl_item->player_uid()))
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
                    first->StartDuel(second);
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
                        
        default:
            _logger.Warning() << "Received undefined packet type";
            break;
        }
        
        _inputMessages.pop();
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
    _respawner.update(delta);
    _monsterSpawner.update(delta);

    // Win condition check
    // TODO: rewrite to work with multiple doors?
    auto doors = _objectsStorage.Subset<Door>();
    auto units = _objectsStorage.Subset<Unit>();
    for(auto unit : units)
    {
        auto& inventory = unit->GetInventory();
        bool has_key = std::any_of(inventory.begin(),
                                   inventory.end(),
                                   [](const std::shared_ptr<Item>& item)
                                   {
                                       return item->GetType() == Item::Type::KEY;
                                   });
        if(has_key && doors[0]->GetPosition() == unit->GetPosition())
        {
            // GAME ENDS
            flatbuffers::FlatBufferBuilder builder;
            auto game_end = CreateSVGameEnd(builder,
                                            unit->GetUID());
            auto msg = CreateMessage(builder,
                                     0,
                                     Messages_SVGameEnd,
                                     game_end.Union());
            builder.Finish(msg);
            _outputEvents.emplace(builder.GetCurrentBufferPointer(),
                                  builder.GetBufferPointer() + builder.GetSize());

            _logger.Info() << "Player with name '" << unit->GetName() << "' won! Escaped from LABYRINTH!";

            _state = State::FINISHED;
        }
    }
}


Point<>
GameWorld::GetRandomPosition()
{
    Point<> point;
    
    bool point_found = false;
    do
    {
        point.x = _randGen.NextInt() % (_mapConf.MapSize * _mapConf.RoomSize + 1);
        point.y = _randGen.NextInt() % (_mapConf.MapSize * _mapConf.RoomSize + 1);
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
