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

    virtual void update(std::chrono::microseconds) override { }

protected:
    Construction(GameWorld& world);
    
    Construction::Type  _constrType;
};


class Door : public Construction
{
public:
    Door(GameWorld& world);
};


class Graveyard : public Construction
{
public:
    Graveyard(GameWorld& world);
};


class Swamp : public Construction
{
public:
    Swamp(GameWorld& world);
};

#endif /* construction_hpp */
