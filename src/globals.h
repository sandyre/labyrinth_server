//
//  globals.h
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 02.02.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef globals_h
#define globals_h

#include <cstdint>

struct Point2
{
    Point2() :
    x(0),
    y(0)
    {
        
    }
    
    Point2(int16_t x_, int16_t y_) :
    x(x_),
    y(y_)
    {
        
    }
    
    bool operator==(const Point2& b)
    {
        return (x == b.x) & (y == b.y);
    }
    
    uint16_t x;
    uint16_t y;
};

#endif /* globals_h */
