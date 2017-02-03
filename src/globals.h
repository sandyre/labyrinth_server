//
//  globals.h
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 02.02.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef globals_h
#define globals_h

struct Vec2
{
    Vec2() :
    x(0),
    y(0)
    {
        
    }
    
    Vec2(int16_t x_, int16_t y_) :
    x(x_),
    y(y_)
    {
        
    }
    
    int16_t x;
    int16_t y;
};

#endif /* globals_h */
