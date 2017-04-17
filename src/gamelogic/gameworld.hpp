//
//  gameworld.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 15.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef gameworld_hpp
#define gameworld_hpp

#include "../globals.h"
#include "gamemap.hpp"
#include "gsnet_generated.h"
#include "../player.hpp"
#include "gameobject.hpp"
#include "units/hero.hpp"
#include "units/air_elementalist.hpp"
#include "units/earth_elementalist.hpp"
#include "units/fire_elementalist.hpp"
#include "units/water_elementalist.hpp"

#include <chrono>
#include <vector>
#include <queue>
#include <random>

class GameWorld
{
public:
    enum State
    {
        RUNNING,
        PAUSE,
        FINISHED
    };
public:
    GameWorld();
    
    virtual void update(std::chrono::milliseconds);
    
    void    AddPlayer(Player);
    void    CreateGameMap(const GameMap::Configuration&);
    void    InitialSpawn();
    
    std::queue<std::vector<uint8_t>>&   GetOutgoingEvents();
    std::queue<std::vector<uint8_t>>&   GetIncomingEvents();
protected:
    void    ApplyInputEvents();
    
    Point2  GetRandomPosition();
protected:
    GameMap::Configuration  m_stMapConf;
    
        // contains outgoing events
    std::queue<std::vector<uint8_t>> m_aInEvents;
    std::queue<std::vector<uint8_t>> m_aOutEvents;
    flatbuffers::FlatBufferBuilder m_oFBuilder;
    
        // basicly contains all objects on scene
    std::vector<GameObject*>    m_apoObjects;
    uint32_t                    m_nObjUIDSeq;
    
        // just a random generator
    std::mt19937    m_oRandGen;
    std::uniform_int_distribution<> m_oRandDistr;
    
    friend GameMap;
    friend Unit;
    friend Hero;
    friend WaterElementalist;
    friend FireElementalist;
    friend EarthElementalist;
    friend AirElementalist;
};

#endif /* gameworld_hpp */
