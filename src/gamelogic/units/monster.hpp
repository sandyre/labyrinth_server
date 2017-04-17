//
//  monster.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef monster_hpp
#define monster_hpp

#include "unit.hpp"

class Monster : public Unit
{
public:
    Monster();
    
    virtual void    update(std::chrono::milliseconds) override {}
};

#endif /* monster_hpp */
