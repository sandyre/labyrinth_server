//
//  gameserver.cpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#include "gameserver.hpp"

#include <Poco/Thread.h>

#include <array>
#include <iostream>

using namespace GameEvent;
using namespace std::chrono;

GameServer::GameServer(const Configuration& config) :
_state(GameServer::State::LOBBY_FORMING),
_config(config),
_startTime(steady_clock::now()),
_msPerUpdate(10)
{
    _serverName = "GS";
    _serverName += std::to_string(_config.Port);

    _logSystem.Init(_serverName,
                    LogSystem::Mode::STDIO);

    _msgBuilder << "Launch configuration {random_seed = " << _config.RandomSeed;
    _msgBuilder << ", lobby_size = " << _config.Players << ", refresh_rate = " << _msPerUpdate.count() << "ms}";
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

void GameServer::SendMulticast(const std::vector<uint8_t>& msg)
{
    std::for_each(_players.cbegin(),
                  _players.cend(),
                  [&msg, this](const Player& player)
                  {
                      _socket.sendTo(msg.data(),
                                     msg.size(),
                                     player.GetAddress());
                  });
}

void GameServer::lobby_forming_stage()
{
    Poco::Net::SocketAddress sender_addr;
    std::array<uint8_t, 512> dataBuffer;

    while(_state == State::LOBBY_FORMING)
    {
        if(_socket.available() > dataBuffer.size())
        {
            auto pack_size = _socket.receiveFrom(dataBuffer.data(),
                                                 dataBuffer.size(),
                                                 sender_addr);
            
            _msgBuilder << "Received packet which size is more than buffer_size. Probably, its a hack or DDoS. Sender addr: " << sender_addr.toString();
            _logSystem.Warning(_msgBuilder.str());
            _msgBuilder.str("");
            continue;
        }
        
        auto pack_size = _socket.receiveFrom(dataBuffer.data(),
                                             dataBuffer.size(),
                                             sender_addr);
        
        flatbuffers::Verifier verifier(dataBuffer.data(),
                                       pack_size);
        if(!VerifyMessageBuffer(verifier))
        {
            _msgBuilder << "Packet verification failed, probably a DDoS. Sender addr: " << sender_addr.toString();
            _logSystem.Warning(_msgBuilder.str());
            _msgBuilder.str("");
            continue;
        }
        
        auto gs_event = GetMessage(dataBuffer.data());

        if(gs_event->event_type() == Events_CLConnection)
        {
            auto con_info = static_cast<const CLConnection *>(gs_event->event());

            auto player = FindPlayerByUID(gs_event->sender_id());
            if(player == _players.end())
            {
                Player new_player(gs_event->sender_id(),
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
                                               0,
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
                                             0,
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
                                         0,
                                         Events_SVPlayerConnected,
                                         gs_new_player.Union());
                _flatBuilder.Finish(gs_event);
                SendMulticast(std::vector<uint8_t>(
                        _flatBuilder.GetBufferPointer(),
                        _flatBuilder.GetBufferPointer() + _flatBuilder.GetSize()));
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
                                             0,
                                             Events_SVHeroPickStage,
                                             gs_heropick.Union());
            _flatBuilder.Finish(gs_event);
            SendMulticast(std::vector<uint8_t>(
                                               _flatBuilder.GetBufferPointer(),
                                               _flatBuilder.GetBufferPointer() + _flatBuilder.GetSize()));
            _flatBuilder.Clear();
        }
    }
}

void GameServer::hero_picking_stage()
{
    Poco::Net::SocketAddress sender_addr;
    std::array<uint8_t, 512> dataBuffer;

    while(_state == State::HERO_PICK)
    {
        if(_socket.available() > dataBuffer.size())
        {
            auto pack_size = _socket.receiveFrom(dataBuffer.data(),
                                                 dataBuffer.size(),
                                                 sender_addr);
            
            _msgBuilder << "Received packet which size is more than buffer_size. Probably, its a hack or DDoS. Sender addr: " << sender_addr.toString();
            _logSystem.Warning(_msgBuilder.str());
            _msgBuilder.str("");
            continue;
        }
        
        auto pack_size = _socket.receiveFrom(dataBuffer.data(),
                                             dataBuffer.size(),
                                             sender_addr);
        
        flatbuffers::Verifier verifier(dataBuffer.data(),
                                       pack_size);
        if(!VerifyMessageBuffer(verifier))
        {
            _msgBuilder << "Packet verification failed, probably a DDoS. Sender addr: " << sender_addr.toString();
            _logSystem.Warning(_msgBuilder.str());
            _msgBuilder.str("");
            continue;
        }
        
        auto gs_event = GetMessage(dataBuffer.data());

        switch(gs_event->event_type())
        {
            case GameEvent::Events_CLHeroPick:
            {
                auto cl_pick = static_cast<const CLHeroPick *>(gs_event->event());

                auto player = FindPlayerByUID(gs_event->sender_id());
                player->SetHeroPicked((Hero::Type) cl_pick->hero_type());

                auto sv_pick  = CreateSVHeroPick(_flatBuilder,
                                                 gs_event->sender_id(),
                                                 cl_pick->hero_type());
                auto sv_event = CreateMessage(_flatBuilder,
                                              0,
                                              Events_SVHeroPick,
                                              sv_pick.Union());
                _flatBuilder.Finish(sv_event);
                SendMulticast(std::vector<uint8_t>(
                        _flatBuilder.GetBufferPointer(),
                        _flatBuilder.GetBufferPointer() + _flatBuilder.GetSize()));
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
                                              0,
                                              Events_SVReadyToStart,
                                              sv_ready.Union());
                _flatBuilder.Finish(sv_event);
                SendMulticast(std::vector<uint8_t>(
                        _flatBuilder.GetBufferPointer(),
                        _flatBuilder.GetBufferPointer() + _flatBuilder.GetSize()));
                _flatBuilder.Clear();
                
                _msgBuilder << "Player " << player->GetNickname() << " is ready";
                _logSystem.Info(_msgBuilder.str());
                _msgBuilder.str("");

                break;
            }

            default:
                _msgBuilder << "Unexpected packet in hero_picking_stage, skipping it";
                _logSystem.Warning(_msgBuilder.str());
                _msgBuilder.str("");
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
    std::array<uint8_t, 512> dataBuffer;

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
                                        0,
                                        Events_SVGenerateMap,
                                        gs_gen_map.Union());
        _flatBuilder.Finish(gs_event);
        SendMulticast(std::vector<uint8_t>(
                _flatBuilder.GetBufferPointer(),
                _flatBuilder.GetBufferPointer() + _flatBuilder.GetSize()));
        _flatBuilder.Clear();

        auto players_ungenerated = _players.size();
        while(players_ungenerated)
        {
            if(_socket.available() > dataBuffer.size())
            {
                auto pack_size = _socket.receiveFrom(dataBuffer.data(),
                                                     dataBuffer.size(),
                                                     sender_addr);
                
                _msgBuilder << "Received packet which size is more than buffer_size. Probably, its a hack or DDoS. Sender addr: " << sender_addr.toString();
                _logSystem.Warning(_msgBuilder.str());
                _msgBuilder.str("");
                continue;
            }
            
            auto pack_size = _socket.receiveFrom(dataBuffer.data(),
                                                 dataBuffer.size(),
                                                 sender_addr);
            
            flatbuffers::Verifier verifier(dataBuffer.data(),
                                           pack_size);
            if(!VerifyMessageBuffer(verifier))
            {
                _msgBuilder << "Packet verification failed, probably a DDoS. Sender addr: " << sender_addr.toString();
                _logSystem.Warning(_msgBuilder.str());
                _msgBuilder.str("");
                continue;
            }
            
            auto gs_event = GetMessage(dataBuffer.data());

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
                                 0,
                                 Events_SVGameStart,
                                 game_start.Union());
        _flatBuilder.Finish(gs_event);
        SendMulticast(std::vector<uint8_t>(
                _flatBuilder.GetBufferPointer(),
                _flatBuilder.GetBufferPointer() + _flatBuilder.GetSize()));
        _flatBuilder.Clear();

        _msgBuilder << "Game begins";
        _logSystem.Info(_msgBuilder.str());
        _msgBuilder.str("");
    }
}

void GameServer::running_game_stage()
{
    std::array<uint8_t, 512>  dataBuffer;
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
            if(_socket.available() > dataBuffer.size())
            {
                auto pack_size = _socket.receiveFrom(dataBuffer.data(),
                                                     dataBuffer.size(),
                                                     sender_addr);
                
                _msgBuilder << "Received packet which size is more than buffer_size. Probably, its a hack or DDoS. Sender addr: " << sender_addr.toString();
                _logSystem.Warning(_msgBuilder.str());
                _msgBuilder.str("");
                continue;
            }
            
            auto pack_size = _socket.receiveFrom(dataBuffer.data(),
                                                 dataBuffer.size(),
                                                 sender_addr);
            
            flatbuffers::Verifier verifier(dataBuffer.data(),
                                           pack_size);
            if(!VerifyMessageBuffer(verifier))
            {
                _msgBuilder << "Packet verification failed, probably a DDoS. Sender addr: " << sender_addr.toString();
                _logSystem.Warning(_msgBuilder.str());
                _msgBuilder.str("");
                continue;
            }
            
            auto gs_event = GetMessage(dataBuffer.data());
            
            auto sender = FindPlayerByUID(gs_event->sender_id());
            if(sender != _players.end())
            {
                if(sender->GetAddress() != sender_addr)
                {
                    _msgBuilder << "Player" << sender->GetNickname() << " dynamic change IP to " << sender_addr.toString();
                    _logSystem.Warning(_msgBuilder.str());
                    _msgBuilder.str("");
                    
                    sender->SetAddress(sender_addr);
                }
                
                _gameWorld->GetIncomingEvents().emplace(dataBuffer.data(),
                                                        dataBuffer.data() + pack_size);
            }
            else
            {
                _msgBuilder << "Received packet from unexisting player with IP [" << sender_addr.toString() << "]";
                _logSystem.Warning(_msgBuilder.str());
                _msgBuilder.str("");
            }
        }

        auto& out_events = _gameWorld->GetOutgoingEvents();
        while(out_events.size())
        {
            auto event = out_events.front();
            SendMulticast(event);
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
