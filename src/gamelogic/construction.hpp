//
//  construction.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef construction_hpp
#define construction_hpp

#include "gameobject.hpp"

#include <string>

class Construction : public GameObject
{
public:
    enum Type
    {
        DOOR = 0x00,
        GRAVEYARD = 0x01,
        SWAMP = 0x02
    };
public:
    Construction::Type  GetType() const
    { return _constrType; }
    
protected:
    Construction();
    
    Construction::Type  _constrType;
};

class Door : public Construction
{
public:
    Door();
};

class Graveyard : public Construction
{
public:
    Graveyard();
};

class Swamp : public Construction
{
public:
    Swamp();
};

#endif /* construction_hpp */
