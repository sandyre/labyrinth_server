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
#include "../../gsnet_generated.h"

#include <string>
#include <vector>
#include <random>

class Rogue : public Hero
{
public:
    Rogue();
    
    virtual void    SpellCast(const GameEvent::CLActionSpell*) override;
    
    virtual void    Attack(const GameEvent::CLActionAttack*) override;
    virtual void    TakeItem(Item*) override;
    
    virtual void    update(std::chrono::milliseconds) override;
protected:
    std::mt19937 m_oRandGen;
    std::uniform_int_distribution<> m_oRandDistr;
};

#endif /* rogue_hpp */
