//
//  hero.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef hero_hpp
#define hero_hpp

#include "unit.hpp"
#include "../item.hpp"
#include "../../gsnet_generated.h"

#include <vector>

class Hero : public Unit
{
public:
    enum Type : int
    {
        FIRST_HERO = 0x00,
        WARRIOR = 0x00,
        MAGE = 0x01,
        ROGUE = 0x02,
        PRIEST = 0x03,
        LAST_HERO = 0x03
    };
public:
    Hero::Type      GetHero() const;
    
    virtual void    update(std::chrono::milliseconds) override;
    
    virtual void                SpellCast(const GameEvent::CLActionSpell*) = 0;

protected:
    Hero();
    
    virtual void        UpdateCDs(std::chrono::milliseconds);
    
protected:
    Hero::Type          m_eHero;
    
    std::vector<std::tuple<bool, std::chrono::milliseconds, std::chrono::milliseconds>> m_aSpellCDs;
};


#endif /* hero_hpp */
