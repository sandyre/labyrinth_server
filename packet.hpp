//
//  packet.hpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#ifndef packet_hpp
#define packet_hpp

struct Packet
{
    enum Type : unsigned char
    {
        PING = 0x00,
        SEARCH_GAME = 0x01
    };
    
    Type    eType;
    int32_t crc32;
};

#pragma pack(push, 1)
struct GamePacket
{
    enum class Type : unsigned char
    {
        PING = 0x01,
        PLAYER_MOVEMENT,
        PLAYER_ATTACK,
        PLAYER_TAKE_ITEM,
        PLAYER_DROP_ITEM,
        PLAYER_DEATH,
        PLAYER_SPAWN,
        MONSTER_SPAWN,
        MONSTER_ATTACK,
    };
    
    GamePacket::Type eType;
    int32_t          nPlayerUID;
    char             aContent[16];
    int32_t          nCRC32;
};
#pragma pack(pop)

struct MoveData
{
    enum Direction : unsigned char
    {
        LEFT = 0x00,
        RIGHT,
        UP,
        DOWN
    };
    
    Direction   eDir;
};

#endif /* packet_hpp */
