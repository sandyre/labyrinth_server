//
//  earth_elementalist.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef earth_elementalist_hpp
#define earth_elementalist_hpp

#include "hero.hpp"

#include <string>
#include <vector>

class EarthElementalist : public Hero
{
public:
    EarthElementalist();
    
    virtual void    CastSpell1() override;
};

#endif /* earth_elementalist_hpp */
