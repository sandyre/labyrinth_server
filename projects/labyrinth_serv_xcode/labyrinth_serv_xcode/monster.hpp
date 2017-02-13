//
//  monster.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 09.02.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef monster_hpp
#define monster_hpp

#include <cstdint>
#include "globals.h"

struct Monster
{
    uint16_t nUID;
    Point2   stPosition;
};

#endif /* monster_hpp */
