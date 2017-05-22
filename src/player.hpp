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
#include <chrono>

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
    m_nHeroIndex(0),
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
    
    const Poco::Net::SocketAddress&   GetAddress() const
    {
        return m_stSocketAddr;
    }
    
    void    SetAddress(Poco::Net::SocketAddress& addr)
    {
        m_stSocketAddr = addr;
    }
    
    int32_t GetHeroPicked() const
    {
        return m_nHeroIndex;
    }
    void    SetHeroPicked(int32_t i)
    {
        m_nHeroIndex = i;
    }
    
    std::chrono::steady_clock::time_point GetLastMsgTimepoint() const
    {
        return m_stLastMsgTimepoint;
    }
    void    SetLastMsgTimepoint(std::chrono::steady_clock::time_point tp)
    {
        m_stLastMsgTimepoint = tp;
    }
protected:
    Player::State               m_eState;
    PlayerUID                   m_nUID;
    std::string                 m_sNickname;
    std::chrono::steady_clock::time_point m_stLastMsgTimepoint;
    Poco::Net::SocketAddress    m_stSocketAddr;
    
    int32_t                     m_nHeroIndex;
};

#endif /* player_hpp */
