//
//  water_elementalist.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef water_elementalist_hpp
#define water_elementalist_hpp

#include "hero.hpp"

#include <string>
#include <vector>
#include <chrono>

class WaterElementalist : public Hero
{
public:
    WaterElementalist();
    
    virtual void    update(std::chrono::milliseconds) override;
    
    virtual void    SpellCast1() override;
    
protected:
    bool    m_bDashing;
    std::chrono::milliseconds   m_nDashDuration;
    std::chrono::milliseconds   m_nDashADuration;
};

#endif /* water_elementalist_hpp */
