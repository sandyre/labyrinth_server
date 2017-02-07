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
#include "globals.h"

// typedefs
using PlayerUID = int32_t;
using ItemUID   = int16_t;
using MonsterUID = int16_t;

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
    PlayerUID       nUID;
    char            aData[50];
};

namespace MSPackets
{
    
struct CLPing
{
    
};
    
struct MSPing
{
    
};
    
struct CLFindGame
{
        // params
};

struct MSGameFound
{
    uint32_t    nGSPort;
};
    
}

namespace GamePackets
{

enum Type : unsigned char
{
        // GENERAL PART (can be sent/received in any server state)
    CL_PING,
    SRV_PING,
    
        // PREGAMING PART (only pregaming state)
    CL_CONNECT, // user connects {uid, name}
    SRV_ACCEPT_CONNECT, // accepts
    SRV_REFUSE_CONNECT, // refuses
    SRV_PLAYER_CONNECTED, // {uid, name} of a new player (if connection is accepted
        // is sent to all players
    CL_PLAYER_READY, // when client hit 'ready' button
    SRV_GEN_MAP, // lobby formed, start generation
    CL_GEN_MAP_OK, // player is ready
    SRV_GAME_START, // game begins
    
        // IN-GAME (verifiable)
    CL_MOVEMENT,
    SRV_MOVEMENT,
    
    CL_TAKE_EQUIP,
    SRV_TAKE_EQUIP,
    
    CL_ATTACK_PLAYER,
    SRV_ATTACK_PLAYER,
    
    CL_ATTACK_MONSTER,
    SRV_ATTACK_MONSTER,
    
    CL_CHECK_WIN,
    SRV_PLAYER_WIN,
    
        // IN-GAME (server only actions)
    SRV_SPAWN_PLAYER,
    SRV_SPAWN_MONSTER,
    SRV_SPAWN_ITEM,
    SRV_SPAWN_CONSTRUCTION,
};
    
/*************************************************
 * GENERAL PACKETS
 *************************************************/

struct CLPing
{
    Type    eType = Type::CL_PING;
};
    
struct SRVPing
{
    Type    eType = Type::SRV_PING;
};
    
/*************************************************
 * PRE-GAME PACKETS
 *************************************************/
    
struct CLConnect
{
    Type        eType = Type::CL_CONNECT;
    PlayerUID   nPlayerUID;
    char        sNickname[16];
};
    
struct SRVAcceptConnect
{
    Type        eType = Type::SRV_ACCEPT_CONNECT;
    PlayerUID   nPlayerUID;
};
    
struct SRVRefuseConnect
{
    Type        eType = Type::SRV_REFUSE_CONNECT;
    PlayerUID   nPlayerUID;
};
    
struct SRVPlayerConnected
{
    Type        eType = Type::SRV_PLAYER_CONNECTED;
    PlayerUID   nPlayerUID;
    char        sNickname[16];
};
    
struct CLPlayerReady
{
    Type        eType = Type::CL_PLAYER_READY;
    PlayerUID   nPlayerUID;
};
    
struct SRVGenMap
{
    Type        eType = Type::SRV_GEN_MAP;
    uint16_t    nChunkN;
    uint32_t    nSeed;
};
    
struct CLGenMapOk
{
    Type        eType = Type::CL_GEN_MAP_OK;
    PlayerUID   nPlayerUID;
};
    
struct SRVGameStart
{
    Type        eType = Type::SRV_GAME_START;
};
    
/*************************************************
 * IN-GAME PACKETS (verifiable)
 *************************************************/

struct CLMovement
{
    Type        eType = Type::CL_MOVEMENT;
    Point2        stPosition;
};

struct SRVMovement
{
    Type        eType = Type::SRV_MOVEMENT;
    PlayerUID   nPlayerUID;
    Point2        stPosition;
};
    
struct CLTakeItem
{
    Type        eType = Type::CL_TAKE_EQUIP;
    PlayerUID   nPlayerUID;
    ItemUID     nItemUID;
};
    
struct SRVTakeItem
{
    Type        eType = Type::SRV_TAKE_EQUIP;
    PlayerUID   nPlayerUID;
    ItemUID     nItemUID;
};
    
struct CLAttackPlayer
{
    Type        eType = Type::CL_ATTACK_PLAYER;
    PlayerUID   nPlayerUID;
    PlayerUID   nAttackedUID;
};
    
struct SRVAttackPlayer
{
    Type        eType = Type::SRV_ATTACK_PLAYER;
    PlayerUID   nPlayerUID;
    PlayerUID   nAttackedUID;
        // attack value, etc
};
    
struct CLCheckWin
{
    Type        eType = Type::CL_CHECK_WIN;
    PlayerUID   nPlayerUID;
};
    
struct SRVPlayerWin
{
    Type        eType = Type::SRV_PLAYER_WIN;
    PlayerUID   nPlayerUID;
};

/*************************************************
 * IN-GAME PACKETS (server-only actions)
 *************************************************/
    
struct SRVSpawnPlayer
{
    Type        eType = Type::SRV_SPAWN_PLAYER;
    PlayerUID   nPlayerUID;
    Point2        stPosition;
};

struct SRVSpawnItem
{
    Type        eType = Type::SRV_SPAWN_ITEM;
    Item::Type  eItemType;
    ItemUID     nItemUID;
    Point2        stPosition;
};
    
struct SRVSpawnConstruction
{
    Type               eType = Type::SRV_SPAWN_CONSTRUCTION;
    Construction::Type eConstrType;
    Point2               stPosition;
};
    
struct SRVSpawnMonster
{
    Type        eType = Type::SRV_SPAWN_MONSTER;
    MonsterUID  nMonsterUID;
    Point2        stPosition;
};

union GamePacket
{
    GamePacket() {}
    struct CLPing  mCLPing;
    struct SRVPing mSRVPing;
    struct CLConnect mCLConnect;
    struct SRVAcceptConnect mSRVAccCon;
    struct SRVRefuseConnect mSRVRefCon;
    struct SRVPlayerConnected mSRVPlCon;
    struct CLPlayerReady mCLPlReady;
    struct SRVGenMap mSRVGenMap;
    struct CLGenMapOk mCLGenMapOk;
    struct SRVGameStart mSRVGameStart;
    
    struct CLMovement mCLMov;
    struct SRVMovement mSRVMov;
    struct CLTakeItem mCLTakeItem;
    struct SRVTakeItem mSRVTakeItem;
    struct CLCheckWin mCLCheckWin;
    struct SRVPlayerWin mSRVPlWin;
};
    
}

#endif /* netpacket_hpp */
