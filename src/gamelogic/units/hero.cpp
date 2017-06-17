//
//  hero.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "hero.hpp"

#include "../../gsnet_generated.h"

#include <chrono>
using namespace std::chrono_literals;

Hero::Hero() :
_heroType(Hero::Type::FIRST_HERO)
{
    _unitType = Unit::Type::HERO;
}

Hero::Type
Hero::GetHero() const
{
    return _heroType;
}

void
Hero::update(std::chrono::microseconds delta)
{
    Unit::update(delta);
}
