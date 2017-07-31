//
//  rogue.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef rogue_hpp
#define rogue_hpp

#include "hero.hpp"
#include "../../GameMessage.h"

#include <string>
#include <vector>
#include <random>


class Rogue
    : public Hero
{
public:
    Rogue(GameWorld& world, uint32_t uid);
    
    virtual void SpellCast(const GameMessage::CLActionSpell*) override;
    
    virtual void TakeItem(std::shared_ptr<Item> item) override;
    
    virtual void update(std::chrono::microseconds) override;
protected:
    std::mt19937 m_oRandGen;
    std::uniform_int_distribution<> m_oRandDistr;
};

#endif /* rogue_hpp */
