//
//  gameserver.cpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#include "gameserver.hpp"

#include "utils/elapsed_time.hpp"

#include <Poco/Thread.h>
#include <Poco/Timer.h>

#include <array>
#include <iostream>

using namespace GameEvent;
using namespace std::chrono;

GameServer::GameServer(const Configuration& config) :
_state(GameServer::State::LOBBY_FORMING),
_config(config),
_startTime(steady_clock::now()),
_msPerUpdate(10),
_pingerTimer(std::make_unique<Poco::Timer>(0, 2000)),
_logger(("GameServer" + std::to_string(_config.Port)), NamedLogger::Mode::STDIO)
{
    _logger.Info() << "Launch configuration {random_seed = " << _config.RandomSeed;
    _logger.Info() << ", lobby_size = " << _config.Players << ", refresh_rate = " << _msPerUpdate.count() << "ms}";
    _logger.Info() << End();

    try
    {
        Poco::Net::SocketAddress addr(Poco::Net::IPAddress(),
                                      _config.Port);
        _socket.bind(addr);
    }
    catch(const std::exception& e)
    {
        _logger.Error() << "Failed to bind socket to port, exception thrown: " << e.what() << End();
        shutdown();
    }

    Poco::TimerCallback<GameServer> free_callback(*this,
                                                  &GameServer::Ping);
    _pingerTimer->start(free_callback);
}

GameServer::~GameServer()
{
    _socket.close();
}

void GameServer::shutdown()
{
    if(_pingerTimer)
        _pingerTimer->stop();
    _state = GameServer::State::FINISHED;
    _logger.Info() << "Finished" << End();
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
    }
    catch(const std::exception& e)
    {
        _logger.Error() << "Unhandled exception thrown in GameServer::run: " << e.what() << End();
        shutdown();
    }
}

void GameServer::Ping(Poco::Timer& timer)
{
    auto ping = CreateSVPing(_flatBuilder);
    auto msg = CreateMessage(_flatBuilder,
                             0,
                             Events_SVPing,
                             ping.Union());
    _flatBuilder.Finish(msg);
    
    SendMulticast(std::vector<uint8_t>(_flatBuilder.GetBufferPointer(),
                                       _flatBuilder.GetBufferPointer() + _flatBuilder.GetSize()));
}

void GameServer::SendMulticast(const std::vector<uint8_t>& msg)
{
    std::lock_guard<std::mutex> lock(_playersMutex);
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
        std::this_thread::sleep_for(_msPerUpdate);
        
        while(_socket.available())
        {
            if(_socket.available() > dataBuffer.size())
            {
                auto pack_size = _socket.receiveFrom(dataBuffer.data(),
                                                     dataBuffer.size(),
                                                     sender_addr);
                
                _logger.Warning() << "Received packet which size is more than buffer_size. Probably, its a hack or DDoS. Sender addr: " << sender_addr.toString() << End();
                continue;
            }
            
            auto pack_size = _socket.receiveFrom(dataBuffer.data(),
                                                 dataBuffer.size(),
                                                 sender_addr);
            
            flatbuffers::Verifier verifier(dataBuffer.data(),
                                           pack_size);
            if(!VerifyMessageBuffer(verifier))
            {
                _logger.Warning() << "Packet verification failed, probably a DDoS. Sender addr: " << sender_addr.toString() << End();
                continue;
            }
            
            auto gs_event = GetMessage(dataBuffer.data());
            auto sender = FindPlayerByUID(gs_event->sender_id());
            if(sender != _players.end()) // FIXME: should be sync'd via mutex. BTW, pingerThread does not modify that shit.
                sender->SetLastMsgTimepoint(steady_clock::now());
            
            switch(gs_event->event_type())
            {
                case GameEvent::Events_CLConnection:
                {
                    auto con_info = static_cast<const CLConnection *>(gs_event->event());
                    auto player = FindPlayerByUID(gs_event->sender_id());
                    
                    if(player == _players.end())
                    {
                        Player new_player(gs_event->sender_id(),
                                          sender_addr,
                                          con_info->nickname()->c_str());
                        new_player.SetLastMsgTimepoint(steady_clock::now());
                        
                        _logger.Info() << "Player \'" << new_player.GetNickname() << "\' [" << sender_addr.toString() << "]"
                        << " connected" << End();
                        
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
                    break;
                }
                
                case GameEvent::Events_CLPing:
                {
                        // we aren't interested in processing this event
                    break;
                }
                default:
                    _logger.Warning() << "Unexpected event received in lobby_forming" << End();
                    break;
            }
        }
        
            // check players last message time (should be less than 2 secs)
        {
            std::lock_guard<std::mutex> lock(_playersMutex);
            _players.erase(std::remove_if(_players.begin(),
                                          _players.end(),
                                          [this](const Player& player)
                                          {
                                              if(duration_cast<milliseconds>(steady_clock::now() - player.GetLastMsgTimepoint()) > 2s)
                                                 {
                                                     _logger.Info() << "Player " << player.GetNickname() << " has been removed from server (reason: no network activity for last 2 secs)" << End();
                                                         // TODO: send msg that player has disconnected
                                                     return true;
                                                 }
                                                 return false;
                                          }),
                                          _players.end());
        }
        
        if(_players.size() == _config.Players)
        {
            _state = GameServer::State::HERO_PICK;
            _logger.Info() << "Starting hero-pick stage" << End();
            
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
        std::this_thread::sleep_for(_msPerUpdate);
        
        while(_socket.available())
        {
            if(_socket.available() > dataBuffer.size())
            {
                auto pack_size = _socket.receiveFrom(dataBuffer.data(),
                                                     dataBuffer.size(),
                                                     sender_addr);
                
                _logger.Warning() << "Received packet which size is more than buffer_size. Probably, its a hack or DDoS. Sender addr: " << sender_addr.toString() << End();
                continue;
            }
            
            auto pack_size = _socket.receiveFrom(dataBuffer.data(),
                                                 dataBuffer.size(),
                                                 sender_addr);
            
            flatbuffers::Verifier verifier(dataBuffer.data(),
                                           pack_size);
            if(!VerifyMessageBuffer(verifier))
            {
                _logger.Warning() << "Packet verification failed, probably a DDoS. Sender addr: " << sender_addr.toString() << End();
                continue;
            }
            
            auto gs_event = GetMessage(dataBuffer.data());
            
            auto sender = FindPlayerByUID(gs_event->sender_id());
            if(sender != _players.end())
                sender->SetLastMsgTimepoint(steady_clock::now());

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
                    
                    _logger.Info() << "Player " << player->GetNickname() << " is ready" << End();
                    break;
                }

                default:
                    _logger.Warning() << "Unexpected packet in hero_picking_stage, skipping it" << End();
                    break;
            }
        }
        
            // check players last message time (should be less than 2 secs)
        {
            std::lock_guard<std::mutex> lock(_playersMutex);
            _players.erase(std::remove_if(_players.begin(),
                                          _players.end(),
                                          [this](const Player& player)
                                          {
                                              if(duration_cast<milliseconds>(steady_clock::now() - player.GetLastMsgTimepoint()) > 2s)
                                              {
                                                  _logger.Warning() << "Player " << player.GetNickname() << " has been removed from server (reason: no network activity for last 2 secs)" << End();
                                                      // TODO: send msg that player has disconnected
                                                  return true;
                                              }
                                              return false;
                                          }),
                           _players.end());
        }
        
        bool bEveryoneReady = std::all_of(_players.begin(),
                                          _players.end(),
                                          [](Player& player)
                                          {
                                              return player.GetState() == Player::State::PRE_READY_TO_START;
                                          });
        
        if(_players.size() != _config.Players)
        {
            throw std::runtime_error("Unhandled situation: num of players < lobby size in HERO_PICKING phase");
        }
        if(bEveryoneReady)
        {
            _state = GameServer::State::GENERATING_WORLD;
            
            _logger.Info() << "Generating world" << End();
            
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

        for(int i = 0; i < _players.size(); ++i)
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
                
                _logger.Warning() << "Received packet which size is more than buffer_size. Probably, its a hack or DDoS. Sender addr: " << sender_addr.toString() << End();
                continue;
            }
            
            auto pack_size = _socket.receiveFrom(dataBuffer.data(),
                                                 dataBuffer.size(),
                                                 sender_addr);
            
            flatbuffers::Verifier verifier(dataBuffer.data(),
                                           pack_size);
            if(!VerifyMessageBuffer(verifier))
            {
                _logger.Warning() << "Packet verification failed, probably a DDoS. Sender addr: " << sender_addr.toString() << End();
                continue;
            }
            
            auto gs_event = GetMessage(dataBuffer.data());

            if(gs_event->event_type() == Events_CLMapGenerated)
            {
                auto cl_gen_ok = static_cast<const CLMapGenerated *>(gs_event->event());

                _logger.Info() << "Player " << cl_gen_ok->player_uid() << " generated map" << End();

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

        _logger.Info() << "Game begins" << End();
    }
}

void GameServer::running_game_stage()
{
    std::array<uint8_t, 512>    dataBuffer;
    Poco::Net::SocketAddress    sender_addr;
    std::chrono::milliseconds   time_no_receive = 0ms;
    ElapsedTime                 frameTime;

    while(_state == State::RUNNING_GAME)
    {
        frameTime.Reset();

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
                
                _logger.Warning() << "Received packet which size is more than buffer_size. Probably, its a hack or DDoS. Sender addr: " << sender_addr.toString() << End();
                continue;
            }
            
            auto pack_size = _socket.receiveFrom(dataBuffer.data(),
                                                 dataBuffer.size(),
                                                 sender_addr);
            
            flatbuffers::Verifier verifier(dataBuffer.data(),
                                           pack_size);
            if(!VerifyMessageBuffer(verifier))
            {
                _logger.Warning() << "Packet verification failed, probably a DDoS. Sender addr: " << sender_addr.toString() << End();
                continue;
            }
            
            auto gs_event = GetMessage(dataBuffer.data());
            
            auto sender = FindPlayerByUID(gs_event->sender_id());
            if(sender != _players.end())
            {
                if(sender->GetAddress() != sender_addr)
                {
                    _logger.Warning() << "Player" << sender->GetNickname() << " dynamic change IP to " << sender_addr.toString() << End();
                    
                    sender->SetAddress(sender_addr);
                }

                    // gameworld shouldnt know about Ping events
                if(gs_event->event_type() != GameEvent::Events_CLPing)
                    _gameWorld->GetIncomingEvents().emplace(dataBuffer.data(),
                                                            dataBuffer.data() + pack_size);
            }
            else
            {
                _logger.Warning() << "Received packet from unexisting player with IP [" << sender_addr.toString() << "]" << End();
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
            _logger.Warning() << "Server timeout exceeded" << End();
            shutdown();
        }

        _gameWorld->update(frameTime.Elapsed<std::chrono::microseconds>());
    }
}

std::vector<Player>::iterator GameServer::FindPlayerByUID(PlayerUID uid)
{
    std::lock_guard<std::mutex> lock(_playersMutex);
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
