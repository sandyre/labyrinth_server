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

#include <queue>

struct InputSequence
{
    InputSequence(int size)
    {
        len = size;
        
        for(auto i = 0;
            i < size;
            ++i)
        {
            sequence.push_back(0);
        }
    }
    
    void    Refresh()
    {
        sequence.clear();
        for(auto i = 0;
            i < len;
            ++i)
        {
            sequence.push_back(0);
        }
    }
    
    int len;
    std::deque<char> sequence;
};

class Monster : public Unit
{
public:
    Monster();
    
    virtual void    update(std::chrono::microseconds) override;
    
    virtual void    Spawn(Point2) override;
    virtual void    Die(Unit * killer) override;
protected:
    std::chrono::microseconds m_msMoveCD;;
    std::chrono::microseconds m_msMoveACD;
    
    Unit * m_pChasingUnit;
    std::queue<Point2> m_pPathToUnit;
    
    std::chrono::microseconds m_msCastTime;
    std::chrono::microseconds m_msACastTime;
    std::vector<InputSequence> m_aCastSequences;
};

#endif /* monster_hpp */
