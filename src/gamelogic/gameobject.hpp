//
//  gameobject.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef gameobject_hpp
#define gameobject_hpp

#include "../globals.h"

#include <chrono>

class GameWorld;

class GameObject
{
public:
    struct Attributes
    {
        static const int MOVABLE = 0x01;
        static const int VISIBLE = 0x02;
        static const int DAMAGABLE = 0x04;
        static const int PASSABLE = 0x08;
    };
public:
    enum Type
    {
        UNDEFINED,
        MAPBLOCK,
        ITEM,
        CONSTRUCTION,
        UNIT
    };
    GameObject();
    ~GameObject();
    
    virtual void        update(std::chrono::microseconds) { } // FIXME: should be pure
    
    GameObject::Type    GetObjType() const;
    uint32_t            GetAttributes() const;
    
    void            SetGameWorld(GameWorld*);
    
    uint32_t        GetUID() const;
    void            SetUID(uint32_t);
    
    Point2          GetLogicalPosition() const;
    void            SetLogicalPosition(Point2);
protected:
        // no need for memory management, gameworld will outlive GO anyway
    GameWorld *         _gameWorld;
    
    GameObject::Type    _objType;
    uint32_t            _objAttributes;
    uint32_t            _UID;
    Point2              _logPos;
};

#endif /* gameobject_hpp */
