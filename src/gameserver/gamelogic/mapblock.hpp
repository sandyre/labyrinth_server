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
    MapBlock(GameWorld& world);
    
    MapBlock::Type  _blockType;
};


class NoBlock : public MapBlock
{
public:
    NoBlock(GameWorld& world);
};


class WallBlock : public MapBlock
{
public:
    WallBlock(GameWorld& world);
};


class BorderBlock : public MapBlock
{
public:
    BorderBlock(GameWorld& world);
};


#endif /* mapblock_hpp */
