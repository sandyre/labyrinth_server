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

class Priest : public Hero
{
public:
    Priest();
    
    virtual void    SpellCast(const GameEvent::CLActionSpell*) override;
    virtual void    update(std::chrono::milliseconds) override;
protected:
    std::chrono::milliseconds m_nRegenInterval;
    std::chrono::milliseconds m_nRegenTimer;
    int16_t m_nRegenAmount;
};

#endif /* priest_hpp */
