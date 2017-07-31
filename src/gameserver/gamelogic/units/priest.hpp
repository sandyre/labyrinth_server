//
//  priest.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef priest_hpp
#define priest_hpp

#include "hero.hpp"

#include <string>
#include <vector>

class Priest
    : public Hero
{
public:
    Priest(GameWorld& world, uint32_t uid);
    
    virtual void SpellCast(const GameMessage::CLActionSpell*) override;
    virtual void update(std::chrono::microseconds) override;

protected:
    std::chrono::microseconds   _regenInterval;
    std::chrono::microseconds   _regenTimer;
    int16_t                     _regenAmount;
};

#endif /* priest_hpp */
