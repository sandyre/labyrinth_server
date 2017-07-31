//
//  hero.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "hero.hpp"


Hero::Hero(GameWorld& world, uint32_t uid)
: Unit(world, uid),
  _heroType(Hero::Type::FIRST_HERO)
{
    _unitType = Unit::Type::HERO;
}


void
Hero::update(std::chrono::microseconds delta)
{
    Unit::update(delta);
}
