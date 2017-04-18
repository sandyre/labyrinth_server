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

#include <vector>

class Hero : public Unit
{
public:
    enum Type : int
    {
        FIRST_HERO = 0x00,
        AIR_ELEMENTALIST = 0x00,
        WATER_ELEMENTALIST = 0x01,
        FIRE_ELEMENTALIST = 0x02,
        EARTH_ELEMENTALIST = 0x03,
        LAST_HERO = 0x03
    };
public:
    Hero::Type      GetHero() const;
    
    virtual void    update(std::chrono::milliseconds) override;
    
    virtual void                SpellCast1() = 0;
    std::chrono::milliseconds   GetSpell1ACD() const;
    bool                        isSpellCast1Ready() const;
protected:
    Hero();
    
    virtual void        UpdateCDs(std::chrono::milliseconds);
    
protected:
    Hero::Type          m_eHero;
    
    std::chrono::milliseconds               m_nSpell1CD;
    std::chrono::milliseconds               m_nSpell1ACD;
};


#endif /* hero_hpp */
