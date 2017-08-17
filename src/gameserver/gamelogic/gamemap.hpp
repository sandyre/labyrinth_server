//
//  gamemap.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef gamemap_hpp
#define gamemap_hpp

#include <stack>
#include <vector>
#include <cstdint>


class GameMapGenerator
{
public:
    enum class MapBlockType : unsigned char
    {
        NOBLOCK = 0x00,
        WALL    = 0x01,
        BORDER  = 0x02
    };
    
    struct Configuration
    {
        uint16_t MapSize;
        uint16_t RoomSize;
        uint32_t Seed;
    };
    
public:
    static std::vector<std::vector<MapBlockType>> GenerateMap(const Configuration& conf);
};

#endif /* gamemap_hpp */
