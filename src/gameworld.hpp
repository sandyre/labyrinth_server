//
//  gameworld.hpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 02.02.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef gameworld_hpp
#define gameworld_hpp

#include <vector>
#include <queue>
#include <map>
#include <cstring>
#include <chrono>
#include "netpacket.hpp"
#include "gamemap.hpp"
#include "player.hpp"
#include "item.hpp"
#include "construction.hpp"
#include "gsnet_generated.h"

class GameWorld
{
public:
    enum State
    {
        RUNNING,
        PAUSE,
        FINISHED
    };
    
    struct Settings
    {
        uint32_t nSeed; // also passed to GameMap generator
        GameMap::Settings stGMSettings;
    };
public:
    GameWorld(GameWorld::Settings&, std::vector<Player>&);
    
    virtual void generate_map();
    virtual void initial_spawn();
    virtual void update(std::chrono::milliseconds);
    
    GameWorld::State        GetState();
    const GameMap&          GetGameMap();
    std::vector<Item>&      GetItems();
    
    std::queue<std::vector<uint8_t>>& GetOutEvents();
    std::queue<std::vector<uint8_t>>& GetInEvents();
    std::vector<Construction>& GetConstructions();
    
protected:
    Point2    GetRandomPosition();
    std::vector<Player>::iterator GetPlayerByUID(PlayerUID);
protected:
    GameWorld::State               m_eState;
    
    flatbuffers::FlatBufferBuilder m_oBuilder;
    GameWorld::Settings    m_stSettings;
    std::queue<std::vector<uint8_t>> m_aInEvents;
    std::queue<std::vector<uint8_t>> m_aOutEvents;
    std::vector<Player>&   m_aPlayers;
    std::vector<Item>    m_aItems;
    std::vector<Construction> m_aConstructions;
//    std::vector<Monster> m_aMonsters;
    GameMap              m_oGameMap;
    
    ItemFactory          m_oItemFactory;
    ConstructionFactory  m_oConstrFactory;
};

#endif /* gameworld_hpp */
