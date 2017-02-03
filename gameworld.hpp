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
    GameWorld(GameWorld::Settings&, std::map<uint32_t, Player>&);
    
    virtual void init();
    virtual void update();
    
    const GameMap&          GetGameMap();
    std::vector<Item>&      GetItems();
    std::queue<GamePacket>& GetEvents();
    std::vector<Construction>& GetConstructions();
    
protected:
    Vec2    GetRandomPosition();
protected:
    GameWorld::Settings    m_stSettings;
    std::queue<GamePacket> m_aEvents;
    std::map<uint32_t, Player>& m_aPlayers;
    std::vector<Item>    m_aItems;
    std::vector<Construction> m_aConstructions;
    GameMap              m_oGameMap;
};

#endif /* gameworld_hpp */
