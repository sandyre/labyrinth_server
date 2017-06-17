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
    
protected:
    MapBlock();
    
    MapBlock::Type  _blockType;
};

class NoBlock : public MapBlock
{
public:
    NoBlock();
};

class WallBlock : public MapBlock
{
public:
    WallBlock();
};

class BorderBlock : public MapBlock
{
public:
    BorderBlock();
};


#endif /* mapblock_hpp */
