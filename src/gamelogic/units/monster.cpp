//
//  monster.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "monster.hpp"

#include "../../gsnet_generated.h"
#include "../gameworld.hpp"

#include <chrono>
using namespace std::chrono_literals;

Monster::Monster() :
m_pChasingUnit(nullptr)
{
    m_eUnitType = Unit::Type::MONSTER;
    m_sName = "Default monster";
    
    m_nBaseDamage = m_nActualDamage = 10;
    m_nMHealth = m_nHealth = 50;
    m_nArmor = 2;
    
    m_msAtkCD = 3s;
    m_msAtkACD = m_msAtkCD;
    
    m_msMoveCD = 2s;
    m_msMoveACD = 0s;
}

void
Monster::update(std::chrono::milliseconds delta)
{
//        // find target to chase
//    if(m_pChasingUnit == nullptr)
//    {
//        for(auto obj : m_poGameWorld->m_apoObjects)
//        {
//            if(obj->GetObjType() == GameObject::Type::UNIT &&
//               (obj->GetAttributes() & GameObject::Attributes::DUELABLE) &&
//               Distance(obj->GetLogicalPosition(), this->GetLogicalPosition()) <= 4.0)
//            {
//                m_pChasingUnit = dynamic_cast<Unit*>(obj);
//                break;
//            }
//        }
//    }
//    else
//    {
//            // firstly check that unit is still in chasable area
//        if(Distance(m_pChasingUnit->GetLogicalPosition(), this->GetLogicalPosition()) <= 4.0)
//        {
//            auto map_size_width = m_poGameWorld->m_stMapConf.nMapSize * m_poGameWorld->m_stMapConf.nRoomSize + 2;
//            std::vector<bool> map_as_flat(map_size_width * map_size_width, false);
//            std::queue<int> path_in_flat;
//            
//                // calculate the path to it
//            auto current_monster_pos = this->GetLogicalPosition().x * map_size_width + this->GetLogicalPosition().y;
//            map_as_flat[current_monster_pos] = true;
//            path_in_flat.push(current_monster_pos);
//            
//            while(!path_in_flat.empty())
//            {
//                m_pPathToUnit.push(Point2(path_in_flat.front() / map_size_width,
//                                          path_in_flat.front() % map_size_width));
//                
//                path_in_flat.pop();
//                
//                    // check adjsmnt points
//            }
//        }
//    }
    
        // TODO: add moving ability
    if(m_eState == Unit::State::DUEL)
    {
        if(m_msAtkACD > 0s)
            m_msAtkACD -= delta;
        
        if((m_pDuelTarget->GetAttributes() & GameObject::Attributes::DAMAGABLE) &&
           m_msAtkACD <= 0s)
        {
            m_msAtkACD = m_msAtkCD;
            this->Attack(nullptr);
        }
    }
}

void
Monster::Spawn(Point2 log_pos)
{
    m_eState = Unit::State::WALKING;
    m_nAttributes = GameObject::Attributes::MOVABLE |
    GameObject::Attributes::VISIBLE |
    GameObject::Attributes::DUELABLE |
    GameObject::Attributes::DAMAGABLE;
    m_nHealth = m_nMHealth;
    
    m_stLogPosition = log_pos;
    
    flatbuffers::FlatBufferBuilder builder;
    auto spawn = GameEvent::CreateSVSpawnMonster(builder,
                                                 this->GetUID(),
                                                 log_pos.x,
                                                 log_pos.y);
    auto msg = GameEvent::CreateMessage(builder,
                                        GameEvent::Events_SVSpawnMonster,
                                        spawn.Union());
    builder.Finish(msg);
    m_poGameWorld->m_aOutEvents.emplace(builder.GetBufferPointer(),
                                        builder.GetBufferPointer() + builder.GetSize());
}
