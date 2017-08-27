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

#include "../../../toolkit/named_logger.hpp"
#include "../../../toolkit/optional.hpp"

#include <deque>


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
    
    void Refresh()
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


class Monster
    : public Unit
{
public:
    Monster(GameWorld& world, uint32_t uid);
    
    virtual void update(std::chrono::microseconds) override;

    virtual void SpellCast(const GameMessage::CLActionSpell*) override { }

    virtual void Spawn(const Point<>& pos) override;
    virtual void Die(const std::string& killerName) override;
    
protected:
    std::chrono::microseconds   _moveCD;;
    std::chrono::microseconds   _moveACD;

    std::experimental::optional<UnitPtr>                _chasingUnit;
    std::experimental::optional<std::deque<Point<>>>    _pathToUnit;

    std::chrono::microseconds   _castTime;
    std::chrono::microseconds   _castATime;
    std::vector<InputSequence>  _castSequence;
};

#endif /* monster_hpp */
