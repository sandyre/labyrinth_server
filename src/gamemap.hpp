//
//  gamemap.hpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 02.02.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef gamemap_hpp
#define gamemap_hpp

#include <vector>
#include "globals.h"
#include <random>

class GameMap
{
public:
    /*
     * MapBlock should be up-to-date with client MapBlock ENUM
     */
    enum class MapBlockType : unsigned char
    {
        NOBLOCK = 0x00,
        WALL    = 0x01,
        BORDER  = 0x02
    };
    
public:
    using Map = std::vector<std::vector<MapBlockType>>;
    
    struct Settings
    {
        int16_t nChunks;
        int16_t nChunkWidth;
        int16_t nChunkHeight;
        uint32_t nSeed;
    };
    
    GameMap();
    GameMap(const GameMap::Settings&);
    ~GameMap();
    
    const Map&          GetMap() const;
    
    Point2              GetRandomPosition();
private:
    GameMap::Settings   m_stSettings;
    Map                 m_oMap;
    std::mt19937        m_oRandGen;
    std::uniform_int_distribution<> m_oRandDistr;
};

#endif /* gamemap_hpp */
