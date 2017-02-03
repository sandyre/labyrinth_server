//
//  netpacket.hpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 01.02.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef netpacket_hpp
#define netpacket_hpp

#include "construction.hpp"
#include "item.hpp"

struct MSPacket
{
    enum class Type : unsigned char
    {
        CL_PING,
        MS_PING,
        
        CL_FIND_GAME,
        MS_GAME_FOUND
    };
    
    MSPacket::Type  eType;
    uint32_t        nUID;
    char            aData[50];
};

namespace MSPackets
{
    
struct CLFindGame
{
        // params
};

struct MSGameFound
{
    uint32_t    nGSPort;
};
    
}

struct GamePacket
{
    enum class Type : unsigned char
    {
        CL_MOVEMENT,
        SRV_MOVEMENT,
        
        CL_TAKE_EQUIP,
        SRV_TOOK_EQUIP,
        
        CL_ATTACK_PLAYER,
        SRV_ATTACK_PLAYER,
        
        CL_ATTACK_MONSTER,
        SRV_ATTACK_MONSTER,
        
        CL_CONNECT,
        SRV_PL_SET_UID,
        SRV_GEN_MAP,
        SRV_SPAWN_PLAYER,
        SRV_SPAWN_MONSTER,
        SRV_SPAWN_ITEM,
        SRV_SPAWN_CONSTRUCTION
    };
    
    GamePacket::Type    eType;
    uint32_t            nUID;
    char                aData[50];
};

namespace GamePackets
{

struct CLMovement
{
    uint16_t    nXCoord;
    uint16_t    nYCoord;
};

struct SRVMovement
{
    uint32_t    nPlayerUID;
    uint16_t    nXCoord;
    uint16_t    nYCoord;
};
    
struct CLTakeItem
{
    uint16_t    nItemID;
};
    
struct SRVTakeItem
{
    uint32_t    nPlayerUID;
    uint16_t    nItemID;
};
    
struct MonsterAttack
{
    uint16_t    nMonsterID;
};
    
struct PlayerAttack
{
    uint16_t    nPlayerID;
};
    
struct SpawnItem
{
    Item::Type  eType;
    uint16_t    nItemID;
    uint16_t    nXCoord;
    uint16_t    nYCoord;
};
    
struct SpawnConstruction
{
    Construction::Type  eType;
    uint16_t            nXCoord;
    uint16_t            nYCoord;
};
    
struct SpawnPlayer
{
    uint32_t nPlayerUID;
    int16_t  nXCoord;
    int16_t  nYCoord;
};
    
struct SpawnMonster
{
    uint16_t nMonsterID;
    int16_t  nXCoord;
    int16_t  nYCoord;
};
    
struct GenMap
{
    uint32_t    nSeed;
};
    
struct PlayerConnect
{
        // to be implemented
};
    
struct SetPlayerUID
{
    uint32_t nUID;
};
    
}

#endif /* netpacket_hpp */
