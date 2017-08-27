//
//  gameobject.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "gameobject.hpp"

#include "gameworld.hpp"


void
GameObject::Move(const Point<>& pos)
{
    _pos = pos;

    auto objects = _world._objectsStorage.Subset<GameObject>();
    for (auto& obj : objects)
    {
        if (obj->GetPosition() == _pos)
        {
            this->OnCollision(obj);
            obj->OnCollision(shared_from_this());
        }
    }
}
