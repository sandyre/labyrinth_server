//
//  player.hpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef player_hpp
#define player_hpp

#include <Poco/Net/SocketAddress.h>
#include <vector>
#include <cassert>

#include "hero.hpp"
#include "globals.h"

class Player
{
public:
    enum class State
    {
        PRE_UNDEFINED,
        PRE_CONNECTED_TO_SERVER,
        PRE_READY_TO_START,
        
        IN_GAME
    };
public:
    Player(PlayerUID uid,
           Poco::Net::SocketAddress addr,
           std::string nickname) :
    m_nUID(uid),
    m_stSocketAddr(addr),
    m_sNickname(nickname),
    
    m_eHeroType(Hero::Type::RANDOM),
    m_eState(Player::State::PRE_UNDEFINED)
    {
        
    }
    
    Player::State   GetState() const
    {
        return m_eState;
    }
    
    void            SetState(Player::State state)
    {
        m_eState = state;
    }
    
    PlayerUID   GetUID() const
    {
        return m_nUID;
    }
    
    std::string GetNickname() const
    {
        return m_sNickname;
    }
    
    Hero *  operator()(int i = 0)
    {
        return m_pHero.get();
    }
    
    const Poco::Net::SocketAddress&   GetAddress() const
    {
        return m_stSocketAddr;
    }
    
    void    SetAddress(Poco::Net::SocketAddress& addr)
    {
        m_stSocketAddr = addr;
    }
    
    void    SetHeroPicked(Hero::Type hero)
    {
        m_eHeroType = hero;
    }
    
    void    CreateHero()
    {
        switch(m_eHeroType)
        {
            case Hero::Type::AIR_ELEMENTALIST:
                m_pHero = std::make_unique<AirElementalist>();
                break;
            case Hero::Type::EARTH_ELEMENTALIST:
                m_pHero = std::make_unique<EarthElementalist>();
                break;
            case Hero::Type::FIRE_ELEMENTALIST:
                m_pHero = std::make_unique<FireElementalist>();
                break;
            case Hero::Type::WATER_ELEMENTALIST:
                m_pHero = std::make_unique<WaterElementalist>();
                break;
            default:
                assert(false);
                break;
        }
    }
protected:
    Player::State               m_eState;
    PlayerUID                   m_nUID;
    std::string                 m_sNickname;
    Poco::Net::SocketAddress    m_stSocketAddr;
    
    Hero::Type                  m_eHeroType;
    std::unique_ptr<Hero>       m_pHero;
};

#endif /* player_hpp */
