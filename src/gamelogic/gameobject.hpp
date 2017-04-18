//
//  gameobject.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef gameobject_hpp
#define gameobject_hpp

#include "globals.h"

#include <chrono>

class GameWorld;

class GameObject
{
public:
    struct Attributes
    {
        static const int MOVABLE = 0x01;
        static const int DUELABLE = 0x02;
        static const int VISIBLE = 0x04;
        static const int DAMAGABLE = 0x08;
        static const int PASSABLE = 0x10;
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
    
    virtual void        update(std::chrono::milliseconds) {}
    
    GameObject::Type    GetObjType() const;
    uint32_t        GetAttributes() const;
    
    void            SetGameWorld(GameWorld*);
    
    uint32_t        GetUID() const;
    void            SetUID(uint32_t);
    
    Point2          GetLogicalPosition() const;
    void            SetLogicalPosition(Point2);
protected:
        // no need for automatic memory management, gameworld will outlive GO anyway
    GameWorld *     m_poGameWorld;
    
    GameObject::Type    m_eObjType;
    uint32_t            m_nAttributes;
    uint32_t            m_nUID;
    Point2              m_stLogPosition;
};

#endif /* gameobject_hpp */
