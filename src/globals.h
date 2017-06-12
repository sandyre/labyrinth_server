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
#include <cmath>

const int GAMEVERSION_MAJOR = 1;
const int GAMEVERSION_MINOR = 1;
const int GAMEVERSION_BUILD = 0;

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
        return (x == b.x) && (y == b.y);
    }
    
    uint16_t x;
    uint16_t y;
};

inline double Distance(const Point2& a, const Point2& b)
{
    return std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}

using PlayerUID = uint32_t;

#endif /* globals_h */
