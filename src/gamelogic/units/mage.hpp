//
//  mage.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef mage_hpp
#define mage_hpp

#include "hero.hpp"

#include <chrono>
#include <string>
#include <vector>

class Mage : public Hero
{
public:
    Mage(GameWorld& world);

    virtual void update(std::chrono::microseconds) override;

    virtual void SpellCast(const GameEvent::CLActionSpell*) override;
};

#endif /* mage_hpp */
