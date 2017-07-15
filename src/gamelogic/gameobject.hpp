//
//  gameobject.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef gameobject_hpp
#define gameobject_hpp

#include "../toolkit/Point.hpp"

#include <chrono>
#include <memory>

class GameWorld;

class GameObject : public std::enable_shared_from_this<GameObject>
{
public:
    struct Attributes
    {
        static const int MOVABLE = 0x01;
        static const int VISIBLE = 0x02;
        static const int DAMAGABLE = 0x04;
        static const int PASSABLE = 0x08;
    };

    enum Type
    {
        UNDEFINED,
        MAPBLOCK,
        ITEM,
        CONSTRUCTION,
        UNIT
    };

public:
    GameObject(GameWorld& world)
    : _objType(GameObject::Type::UNDEFINED),
      _uid(0),
      _objAttributes(0),
      _world(world)
    {
        _objAttributes |= GameObject::Attributes::VISIBLE;
    }
    
    virtual void update(std::chrono::microseconds) = 0;
    
    GameObject::Type GetType() const
    { return _objType; }

    uint32_t GetAttributes() const
    { return _objAttributes; }
    
    uint32_t GetUID() const
    { return _uid; }

    void SetUID(uint32_t uid)
    { _uid = uid; }
    
    Point<> GetPosition() const
    { return _pos; }

    void SetPosition(Point<> pos)
    { _pos = pos; }
    
protected:
    GameWorld&          _world;
    
    GameObject::Type    _objType;
    uint32_t            _objAttributes;
    uint32_t            _uid;
    Point<>               _pos;
};

#endif /* gameobject_hpp */
