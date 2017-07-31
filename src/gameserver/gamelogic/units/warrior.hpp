//
//  warrior.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef warrior_hpp
#define warrior_hpp

#include "hero.hpp"

#include <string>
#include <vector>


class Warrior
    : public Hero
{
public:
    Warrior(GameWorld& world, uint32_t uid);
    
    virtual void SpellCast(const GameMessage::CLActionSpell*) override;
};

#endif /* warrior_hpp */
