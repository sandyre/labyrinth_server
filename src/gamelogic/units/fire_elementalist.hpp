//
//  fire_elementalist.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef fire_elementalist_hpp
#define fire_elementalist_hpp

#include "hero.hpp"

#include <string>
#include <vector>

class FireElementalist : public Hero
{
public:
    FireElementalist();
    
    virtual void    CastSpell1() override;
};

#endif /* fire_elementalist_hpp */
