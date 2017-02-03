//
//  gameworld.cpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 02.02.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#include "gameworld.hpp"

#include <random>

GameWorld::GameWorld(GameWorld::Settings& sets,
                     std::map<uint32_t, Player>& players) :
m_stSettings(sets),
m_aPlayers(players)
{
    sets.stGMSettings.nSeed = sets.nSeed;
}

void
GameWorld::init()
{
    m_oGameMap = GameMap(m_stSettings.stGMSettings);
    
        // spawn key
    Vec2 random_pos = GetRandomPosition();
    Item item;
    item.eType = Item::Type::KEY;
    item.nUID    = 0;
    item.nXCoord = random_pos.x;
    item.nYCoord = random_pos.y;
    m_aItems.push_back(item);
    
    GamePacket item_spawn;
    item_spawn.eType = GamePacket::Type::SRV_SPAWN_ITEM;
    
    GamePackets::SpawnItem sp_item;
    sp_item.eType   = item.eType;
    sp_item.nItemID = item.nUID;
    sp_item.nXCoord = item.nXCoord;
    sp_item.nYCoord = item.nYCoord;
    std::memcpy(item_spawn.aData, &sp_item, sizeof(sp_item));
    
    m_aEvents.push(item_spawn);
    
        // spawn door
    item_spawn.eType = GamePacket::Type::SRV_SPAWN_CONSTRUCTION;
    random_pos = GetRandomPosition();
    Construction door;
    door.eType = Construction::Type::DOOR;
    door.nXCoord = random_pos.x;
    door.nYCoord = random_pos.y;
    m_aConstructions.push_back(door);
    
    GamePackets::SpawnConstruction sp_constr;
    sp_constr.eType = door.eType;
    sp_constr.nXCoord = door.nXCoord;
    sp_constr.nYCoord = door.nYCoord;
    std::memcpy(item_spawn.aData, &sp_constr, sizeof(sp_constr));
    
    m_aEvents.push(item_spawn);
}

void
GameWorld::update()
{
        // Monsters logic, Items spawning, etc.
}

const GameMap&
GameWorld::GetGameMap()
{
    return m_oGameMap;
}

std::vector<Item>&
GameWorld::GetItems()
{
    return m_aItems;
}

std::vector<Construction>&
GameWorld::GetConstructions()
{
    return m_aConstructions;
}

std::queue<GamePacket>&
GameWorld::GetEvents()
{
    return m_aEvents;
}

Vec2
GameWorld::GetRandomPosition()
{
    Vec2 position;

    bool  bIsEngaged = false;
    do
    {
        position = m_oGameMap.GetRandomPosition();
        bIsEngaged = false;
        
        for(auto& player : m_aPlayers)
        {
            if(player.second.nXCoord == position.x &&
               player.second.nYCoord == position.y)
            {
                bIsEngaged = true;
                break;
            }
        }
        
        for(auto& item : m_aItems)
        {
            if(item.nXCoord == position.x &&
               item.nYCoord == position.y)
            {
                bIsEngaged = true;
                break;
            }
        }
        
        for(auto& constr : m_aConstructions)
        {
            if(constr.nXCoord == position.x &&
               constr.nYCoord == position.y)
            {
                bIsEngaged = true;
                break;
            }
        }
    } while(bIsEngaged != false);
    
    return position;
}
