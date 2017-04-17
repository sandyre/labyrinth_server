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

class GameWorld;

class GameMap
{
    enum class MapBlockType : unsigned char
    {
        NOBLOCK = 0x00,
        WALL    = 0x01,
        BORDER  = 0x02
    };
public:
    struct Configuration
    {
        uint16_t nMapSize;
        uint16_t nRoomSize;
        uint32_t nSeed;
    };
    
    GameMap();
    ~GameMap();
    
    void    GenerateMap(const Configuration&, GameWorld*);
};

#endif /* gamemap_hpp */
