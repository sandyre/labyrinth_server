//
//  gameobject.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef gameobject_hpp
#define gameobject_hpp

#include "../../toolkit/Point.hpp"

#include <chrono>
#include <memory>
#include <string>


class GameWorld;

class GameObject
    : public std::enable_shared_from_this<GameObject>
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
    GameObject(GameWorld& world, uint32_t uid)
    : _objType(GameObject::Type::UNDEFINED),
      _uid(uid),
      _objAttributes(),
      _world(world)
    {
        _objAttributes |= GameObject::Attributes::VISIBLE;
    }
    
    GameObject::Type GetType() const
    { return _objType; }

    std::string GetName() const
    { return _name; }

    void SetName(const std::string& name)
    { _name = name; }

    uint32_t GetAttributes() const
    { return _objAttributes; }
    
    uint32_t GetUID() const
    { return _uid; }
    
    Point<> GetPosition() const
    { return _pos; }

    void SetPosition(Point<> pos)
    { _pos = pos; }

    virtual void update(std::chrono::microseconds) = 0;

    virtual void OnCollision(const std::shared_ptr<GameObject>& object)
    { }

    virtual void Spawn(const Point<>& pos)
    { _pos = pos; }
    
    virtual void Destroy()
    { } // server side has nothing to do with destroy. for consistency with client API
    
protected:
    GameWorld&          _world;
    GameObject::Type    _objType;
    uint32_t            _uid;
    std::string         _name;
    uint32_t            _objAttributes;
    Point<>             _pos;
};
using GameObjectPtr = std::shared_ptr<GameObject>;

#endif /* gameobject_hpp */
