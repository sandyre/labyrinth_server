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
#include "netpacket.hpp"
#include "gamemap.hpp"
#include "player.hpp"
#include "item.hpp"
#include "construction.hpp"

class GameWorld
{
public:
    struct Settings
    {
            // to be implemented
        uint32_t nSeed; // also passed to GameMap generator
        GameMap::Settings stGMSettings;
    };
public:
    GameWorld(GameWorld::Settings&, std::vector<Player>&);
    
    virtual void init();
    virtual void update(std::chrono::milliseconds);
    
    const GameMap&          GetGameMap();
    std::vector<Item>&      GetItems();
    std::queue<GamePackets::GamePacket>& GetEvents();
    std::vector<Construction>& GetConstructions();
    
protected:
    Point2    GetRandomPosition();
protected:
    const int   m_nItemSpawnRate = 3000; // 3 seconds
    int         m_nItemSpawnTimer = 0;
    
    GameWorld::Settings    m_stSettings;
    std::queue<GamePackets::GamePacket> m_aEvents;
    std::vector<Player>&   m_aPlayers;
    std::vector<Item>    m_aItems;
    std::vector<Construction> m_aConstructions;
    GameMap              m_oGameMap;
};

#endif /* gameworld_hpp */
