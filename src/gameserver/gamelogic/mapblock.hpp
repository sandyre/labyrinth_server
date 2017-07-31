//
//  mapblock.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef mapblock_hpp
#define mapblock_hpp

#include "gameobject.hpp"

#include <string>


class MapBlock : public GameObject
{
public:
    enum class Type
    {
        NOBLOCK,
        WALL,
        BORDER
    };
    
public:
    MapBlock::Type  GetType() const;

    virtual void update(std::chrono::microseconds) override { }
    
protected:
    MapBlock(GameWorld& world, uint32_t uid);
    
    MapBlock::Type  _blockType;
};


class NoBlock : public MapBlock
{
public:
    NoBlock(GameWorld& world, uint32_t uid);
};


class WallBlock : public MapBlock
{
public:
    WallBlock(GameWorld& world, uint32_t uid);
};


class BorderBlock : public MapBlock
{
public:
    BorderBlock(GameWorld& world, uint32_t uid);
};


#endif /* mapblock_hpp */
