//
//  gameserver.cpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#include "gameserver.hpp"

#include "gsnet_generated.h"
#include "gamelogic/gamemap.hpp"

#include <Poco/Thread.h>

#include <algorithm>
#include <iostream>
#include <memory>

using namespace GameEvent;
using namespace std::chrono;

GameServer::GameServer(const Configuration& config) :
_state(GameServer::State::LOBBY_FORMING),
_config(config), _startTime(steady_clock::now()),
_msPerUpdate(2)
{
    _serverName = "GameServer ";
    _serverName += std::to_string(_config.Port);

    _logSystem.Init(_serverName,
                    LogSystem::Mode::STDIO);

    _msgBuilder << "Started. Configuration: {SEED:" << _config.RandomSeed;
    _msgBuilder << "; PLAYERS_COUNT: " << _config.Players << "}";
    _logSystem.Info(_msgBuilder.str());
    _msgBuilder.str("");

    Poco::Net::SocketAddress addr(Poco::Net::IPAddress(),
                                  _config.Port);
    _socket.bind(addr);
}

GameServer::~GameServer()
{
    _socket.close();
    _logSystem.Close();
}

GameServer::State GameServer::GetState() const
{
    return _state;
}

GameServer::Configuration GameServer::GetConfig() const
{
    return _config;
}

void GameServer::shutdown()
{
    _state = GameServer::State::FINISHED;
    _msgBuilder << "Finished";
    _logSystem.Info(_msgBuilder.str());
    _msgBuilder.str("");
}

void GameServer::run()
{
    try
    {
        // lobby forming stage
        lobby_forming_stage();
        // hero picking stage
        hero_picking_stage();
        // worldgen stage
        world_generation_stage();
        // running game stage
        running_game_stage();
    } catch(std::exception& e)
    {
        _msgBuilder << e.what();
        _logSystem.Error(_msgBuilder.str());
        _msgBuilder.str("");
        shutdown();
    }
}

void GameServer::SendToAll(uint8_t * buf,
                           size_t size)
{
    std::for_each(_players.cbegin(),
                  _players.cend(),
                  [buf, size, this](const Player& player)
                  {
                      _socket.sendTo(buf,
                                     size,
                                     player.GetAddress());
                  });
}

void GameServer::lobby_forming_stage()
{
    Poco::Net::SocketAddress sender_addr;
    char                     buf[256];

    while(_state == State::LOBBY_FORMING)
    {
        auto size = _socket.receiveFrom(buf,
                                        256,
                                        sender_addr);

        auto gs_event = GetMessage(buf);

        if(gs_event->event_type() == Events_CLConnection)
        {
            auto con_info = static_cast<const CLConnection *>(gs_event->event());

            auto player = FindPlayerByUID(con_info->player_uid());
            if(player == _players.end())
            {
                Player new_player(con_info->player_uid(),
                                  sender_addr,
                                  con_info->nickname()->c_str());

                _msgBuilder << "Player \'" << new_player.GetNickname() << "\' [" << sender_addr.toString() << "]"
                            << " connected";
                _logSystem.Info(_msgBuilder.str());
                _msgBuilder.str("");

                _players.emplace_back(std::move(new_player));

                // notify connector that he is accepted
                auto gs_accept = CreateSVConnectionStatus(_flatBuilder,
                                                          ConnectionStatus_ACCEPTED);
                auto gs_event  = CreateMessage(_flatBuilder,
                                               Events_SVConnectionStatus,
                                               gs_accept.Union());
                _flatBuilder.Finish(gs_event);

                _socket.sendTo(_flatBuilder.GetBufferPointer(),
                               _flatBuilder.GetSize(),
                               sender_addr);
                _flatBuilder.Clear();

                // send him info about all current players in lobby
                for(auto& player : _players)
                {
                    auto nick        = _flatBuilder.CreateString(player.GetNickname());
                    auto player_info = CreateSVPlayerConnected(_flatBuilder,
                                                               player.GetUID(),
                                                               nick);
                    gs_event = CreateMessage(_flatBuilder,
                                             Events_SVPlayerConnected,
                                             player_info.Union());
                    _flatBuilder.Finish(gs_event);

                    _socket.sendTo(_flatBuilder.GetBufferPointer(),
                                   _flatBuilder.GetSize(),
                                   sender_addr);
                    _flatBuilder.Clear();
                }

                // notify all players about connected one
                auto nick          = _flatBuilder.CreateString(con_info->nickname()->c_str(),
                                                               con_info->nickname()->size());
                auto gs_new_player = CreateSVPlayerConnected(_flatBuilder,
                                                             con_info->player_uid(),
                                                             nick);
                gs_event = CreateMessage(_flatBuilder,
                                         Events_SVPlayerConnected,
                                         gs_new_player.Union());
                _flatBuilder.Finish(gs_event);

                SendToAll(_flatBuilder.GetBufferPointer(),
                          _flatBuilder.GetSize());
                _flatBuilder.Clear();
            }
        }

        if(_players.size() == _config.Players)
        {
            _state = GameServer::State::HERO_PICK;

            _msgBuilder << "Starting hero-pick stage";
            _logSystem.Info(_msgBuilder.str());
            _msgBuilder.str("");
            
            auto gs_heropick = CreateSVHeroPickStage(_flatBuilder);
            auto gs_event    = CreateMessage(_flatBuilder,
                                             Events_SVHeroPickStage,
                                             gs_heropick.Union());
            _flatBuilder.Finish(gs_event);

            SendToAll(_flatBuilder.GetBufferPointer(),
                      _flatBuilder.GetSize());
            _flatBuilder.Clear();
        }
    }
}

void GameServer::hero_picking_stage()
{
    Poco::Net::SocketAddress sender_addr;
    char                     buf[256];

    while(_state == State::HERO_PICK)
    {
        auto size = _socket.receiveFrom(buf,
                                        256,
                                        sender_addr);

        auto gs_event = GetMessage(buf);

        switch(gs_event->event_type())
        {
            case GameEvent::Events_CLHeroPick:
            {
                auto cl_pick = static_cast<const CLHeroPick *>(gs_event->event());

                auto player = FindPlayerByUID(cl_pick->player_uid());
                player->SetHeroPicked((Hero::Type) cl_pick->hero_type());

                auto sv_pick  = CreateSVHeroPick(_flatBuilder,
                                                 cl_pick->player_uid(),
                                                 cl_pick->hero_type());
                auto sv_event = CreateMessage(_flatBuilder,
                                              Events_SVHeroPick,
                                              sv_pick.Union());
                _flatBuilder.Finish(sv_event);
                SendToAll(_flatBuilder.GetBufferPointer(),
                          _flatBuilder.GetSize());
                _flatBuilder.Clear();

                break;
            }

            case GameEvent::Events_CLReadyToStart:
            {
                auto cl_ready = static_cast<const CLReadyToStart *>(gs_event->event());

                auto player = FindPlayerByUID(cl_ready->player_uid());
                player->SetState(Player::State::PRE_READY_TO_START);

                auto sv_ready = CreateSVReadyToStart(_flatBuilder,
                                                     cl_ready->player_uid());
                auto sv_event = CreateMessage(_flatBuilder,
                                              Events_SVReadyToStart,
                                              sv_ready.Union());
                _flatBuilder.Finish(sv_event);
                SendToAll(_flatBuilder.GetBufferPointer(),
                          _flatBuilder.GetSize());
                _flatBuilder.Clear();
                
                _msgBuilder << "Player " << player->GetNickname() << " is ready";
                _logSystem.Info(_msgBuilder.str());
                _msgBuilder.str("");

                break;
            }

            default:
                assert(false);
                break;
        }

        bool bEveryoneReady = std::all_of(_players.begin(),
                                          _players.end(),
                                          [](Player& player)
                                          {
                                              return player.GetState() == Player::State::PRE_READY_TO_START;
                                          });
        if(bEveryoneReady)
        {
            _state = GameServer::State::GENERATING_WORLD;

            _msgBuilder << "Generating world";
            _logSystem.Info(_msgBuilder.str());
            _msgBuilder.str("");

            for(auto& player : _players)
            {
                player.SetState(Player::State::IN_GAME);
            }
        }
    }
}

void GameServer::world_generation_stage()
{
    Poco::Net::SocketAddress sender_addr;
    char                     buf[256];

    while(_state == State::GENERATING_WORLD)
    {
        GameMap::Configuration sets;
        sets.Seed     = _config.RandomSeed;
        sets.Seed     = _config.RandomSeed;
        sets.MapSize  = 3;
        sets.RoomSize = 10;
        _gameWorld = std::make_unique<GameWorld>();
        _gameWorld->CreateGameMap(sets);

        for(int i = 0;
            i < _players.size();
            ++i)
        {
            _gameWorld->AddPlayer(_players[i]);
        }

        _gameWorld->InitialSpawn();

        auto gs_gen_map = CreateSVGenerateMap(_flatBuilder,
                                              sets.MapSize,
                                              sets.RoomSize,
                                              sets.Seed);
        auto gs_event   = CreateMessage(_flatBuilder,
                                        Events_SVGenerateMap,
                                        gs_gen_map.Union());
        _flatBuilder.Finish(gs_event);

        SendToAll(_flatBuilder.GetBufferPointer(),
                  _flatBuilder.GetSize());
        _flatBuilder.Clear();

        auto players_ungenerated = _players.size();
        while(players_ungenerated)
        {
            _socket.receiveBytes(buf,
                                 256);

            auto gs_event = GetMessage(buf);

            if(gs_event->event_type() == Events_CLMapGenerated)
            {
                auto cl_gen_ok = static_cast<const CLMapGenerated *>(gs_event->event());

                _msgBuilder << "Player " << cl_gen_ok->player_uid();
                _msgBuilder << " generated map";
                _logSystem.Info(_msgBuilder.str());
                _msgBuilder.str("");

                --players_ungenerated;
            }
        }

        _state = GameServer::State::RUNNING_GAME;

        // notify players that game starts!
        auto game_start = CreateSVGameStart(_flatBuilder);
        gs_event = CreateMessage(_flatBuilder,
                                 Events_SVGameStart,
                                 game_start.Union());
        _flatBuilder.Finish(gs_event);
        SendToAll(_flatBuilder.GetBufferPointer(),
                  _flatBuilder.GetSize());
        _flatBuilder.Clear();

        _msgBuilder << "Game begins";
        _logSystem.Info(_msgBuilder.str());
        _msgBuilder.str("");
    }
}

void GameServer::running_game_stage()
{
    char                      buf[256];
    Poco::Net::SocketAddress  sender_addr;
    std::chrono::milliseconds time_no_receive = 0ms;

    while(_state == State::RUNNING_GAME)
    {
        auto frame_start = steady_clock::now();

        // sleep for some time, then get all packets and pass it to the gameworld, update
        std::this_thread::sleep_for(_msPerUpdate);

        if(!_socket.available())
        {
            time_no_receive += _msPerUpdate;
        }
        while(_socket.available())
        {
            time_no_receive = 0ms;
            bool event_valid = false;
            auto pack_size   = _socket.receiveFrom(buf,
                                                   256,
                                                   sender_addr);

            auto gs_event = GetMessage(buf);

            switch(gs_event->event_type())
            {
                case Events_CLActionMove:
                {
                    auto cl_mov = static_cast<const CLActionMove *>(gs_event->event());
                    auto player = FindPlayerByUID(cl_mov->target_uid());

                    if(player != _players.end() && player->GetAddress() != sender_addr)
                    {
                        player->SetAddress(sender_addr);
                    }

                    event_valid = true;
                    break;
                }

                case Events_CLActionItem:
                {
                    auto cl_item = static_cast<const CLActionItem *>(gs_event->event());
                    auto player  = FindPlayerByUID(cl_item->player_uid());

                    if(player != _players.end() && player->GetAddress() != sender_addr)
                    {
                        player->SetAddress(sender_addr);
                    }

                    event_valid = true;
                    break;
                }

                case Events_CLActionDuel:
                {
                    auto cl_duel = static_cast<const CLActionDuel *>(gs_event->event());

                    //FIXME: players validation needed?

                    event_valid = true;
                    break;
                }

                case Events_CLActionSpell:
                {
                    //FIXME: validation needed, at least for players id
                    event_valid = true;
                    break;
                }

                case Events_CLRequestWin:
                {
                    event_valid = true;
                    break;
                }

                default:
                    assert(false);
                    break;
            }

            if(event_valid) // add received event to the gameworld
            {
                auto& in_events = _gameWorld->GetIncomingEvents();
                std::vector<uint8_t> event(buf,
                                           buf + pack_size);
                in_events.emplace(event);
            }
        }

        auto& out_events = _gameWorld->GetOutgoingEvents();
        while(out_events.size())
        {
            auto event = out_events.front();
            SendToAll(event.data(),
                      event.size());
            out_events.pop();
        }

        if(time_no_receive >= 180s)
        {
            _msgBuilder << "Server timeout exceeded.";
            _logSystem.Warning(_msgBuilder.str());
            _msgBuilder.str("");
            shutdown();
        }

        auto frame_end = steady_clock::now();

        _gameWorld->update(duration_cast<microseconds>(frame_end - frame_start));
    }
}

void GameServer::SendToOne(uint32_t player_id,
                           uint8_t * buf,
                           size_t size)
{
    auto player = FindPlayerByUID(player_id);

    if(player != _players.end())
    {
        _socket.sendTo(buf,
                       size,
                       player->GetAddress());
    }
}

std::vector<Player>::iterator GameServer::FindPlayerByUID(PlayerUID uid)
{
    for(auto iter = _players.begin();
        iter != _players.end();
        ++iter)
    {
        if((*iter).GetUID() == uid)
        {
            return iter;
        }
    }

    return _players.end();
}
