//
//  air_elementalist.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef air_elementalist_hpp
#define air_elementalist_hpp

#include "hero.hpp"

#include <string>
#include <vector>

class AirElementalist : public Hero
{
public:
    AirElementalist();
    
    virtual void    CastSpell1() override;
    
    virtual void    TakeItem(Item*) override;
    
    virtual void    update(std::chrono::milliseconds) override;
protected:
        // Spell cast 1: invisibility
    std::chrono::milliseconds   m_nInvisADuration;
    std::chrono::milliseconds   m_nInvisDuration;
};

#endif /* air_elementalist_hpp */
