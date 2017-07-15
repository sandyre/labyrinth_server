//
//  gameserver.cpp
//  labyrinth_server
//
//  Created by Aleksandr Borzikh on 28.01.17.
//  Copyright © 2017 sandyre. All rights reserved.
//

#include "gameserver.hpp"

#include "../toolkit/elapsed_time.hpp"
#include "../toolkit/SafePacketGetter.hpp"

#include <Poco/Thread.h>
#include <Poco/Timer.h>

#include <array>
#include <iostream>

using namespace GameMessage;
using namespace std::chrono;

using IPAddress = Poco::Net::SocketAddress;
using Clock = std::chrono::steady_clock;

template<typename T>
using optional = std::experimental::optional<T>;

class GameServer::PlayerConnection
{
public:
    enum class ConnectionStatus
    {
        ACTIVE,
        TIMEOUT
    };

public:
    PlayerConnection(uint32_t uuid,
                     uint32_t localUuid,
                     const std::string& name,
                     const IPAddress& ip)
    : _uuid(uuid),
      _localUUID(localUuid),
      _name(name),
      _ip(ip),
      _timepoint(Clock::now())
    { }

    ConnectionStatus GetConnectionStatus() const
    {
        if(duration_cast<milliseconds>(Clock::now() - _timepoint) > 5s)
            return ConnectionStatus::TIMEOUT;

        return ConnectionStatus::ACTIVE;
    }

    uint32_t GetUUID() const
    { return _uuid; }

    uint32_t GetLocalUID() const
    { return _localUUID; }

    const std::string& GetName() const
    { return _name; }

    void SetAddress(const IPAddress& ip)
    {
        if(_ip != ip)
            _ip = ip;
    }

    const IPAddress& GetAddress() const
    { return _ip; }

    void SetLastPacketTimepoint(const Clock::time_point& timepoint)
    { _timepoint = timepoint; }

    const Clock::time_point& GetLastPacketTimepoint() const
    { return _timepoint; }

private:
    uint32_t            _uuid;
    uint32_t            _localUUID;
    std::string         _name;
    IPAddress           _ip;
    Clock::time_point   _timepoint;
};

GameServer::GameServer(const Configuration& config)
: Task(("GameServer" + std::to_string(config.Port))),
  _state(GameServer::State::LOBBY_FORMING),
  _config(config),
  _msPerUpdate(10),
  _pingerTimer(std::make_unique<Poco::Timer>(0, 2000)),
  _logger("Server", NamedLogger::Mode::STDIO)
{
    _logger.Info() << "Launch configuration {random_seed = " << _config.RandomSeed
            << ", lobby_size = " << _config.Players << ", refresh_rate = " << _msPerUpdate.count() << "ms}";

    try
    {
        Poco::Net::SocketAddress addr(Poco::Net::IPAddress(),
                                      _config.Port);
        _socket.bind(addr);
    }
    catch(const std::exception& e)
    {
        _logger.Error() << "Failed to bind socket to port, exception thrown: " << e.what();
        shutdown();
    }

    Poco::TimerCallback<GameServer> free_callback(*this,
                                                  &GameServer::Ping);
    _pingerTimer->start(free_callback);

    Task::setState(Poco::Task::TaskState::TASK_IDLE);
}

void GameServer::shutdown()
{
    if(_pingerTimer)
        _pingerTimer->stop();

    Task::setState(Poco::Task::TaskState::TASK_FINISHED);
}

void GameServer::runTask()
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
        _logger.Error() << "Unhandled exception thrown in GameServer::run: " << e.what();
        shutdown();
    }
}

void GameServer::Ping(Poco::Timer& timer)
{
    static flatbuffers::FlatBufferBuilder builder;
    auto ping = CreateSVPing(builder);
    auto msg = CreateMessage(builder,
                             0,
                             Messages_SVPing,
                             ping.Union());
    builder.Finish(msg);

    SendMulticast(builder);
}

void GameServer::SendSingle(flatbuffers::FlatBufferBuilder& builder,
                            Poco::Net::SocketAddress& address)
{
    _socket.sendTo(builder.GetBufferPointer(),
                   builder.GetSize(),
                   address);
    builder.Clear();
}

void GameServer::SendMulticast(flatbuffers::FlatBufferBuilder& builder)
{
    std::lock_guard<std::recursive_mutex> lock(_playersMutex);
    std::for_each(_playersConnections.cbegin(),
                  _playersConnections.cend(),
                  [&builder, this](const PlayerConnection& player)
                  {
                      _socket.sendTo(builder.GetBufferPointer(),
                                     builder.GetSize(),
                                     player.GetAddress());
                  });
    builder.Clear();
}

void GameServer::SendMulticast(const std::vector<uint8_t>& buffer)
{
    std::lock_guard<std::recursive_mutex> lock(_playersMutex);
    std::for_each(_playersConnections.cbegin(),
                  _playersConnections.cend(),
                  [&buffer, this](const PlayerConnection& player)
                  {
                      _socket.sendTo(buffer.data(),
                                     buffer.size(),
                                     player.GetAddress());
                  });
}

void GameServer::lobby_forming_stage()
{
    while(_state == State::LOBBY_FORMING)
    {
        std::this_thread::sleep_for(_msPerUpdate);

        while(_socket.available())
        {
            SafePacketGetter packetGetter(_socket);
            auto packet = packetGetter.Get<GameMessage::Message>();
            if(!packet)
                continue;
            
            auto message = GetMessage(packet->Data.data());
            auto senderIp = packet->Sender;

            switch(message->payload_type())
            {
            case GameMessage::Messages_CLConnection:
            {
                auto con_info = static_cast<const CLConnection *>(message->payload());

                if(PlayerExists(con_info->player_uid()))
                {
                    _logger.Warning() << "Player, which has already been added into lobby, tried to connect twice";
                    continue;
                }

                    // FIXME: should make Builder for PlayerConnection to validate input
                PlayerConnection playerConnection(con_info->player_uid(),
                                                  con_info->player_uid(), // TODO: generate local UUID randomly
                                                  std::string(con_info->nickname()->c_str()),
                                                  senderIp);
                _playersConnections.push_back(std::move(playerConnection));

                    // Notify player that he is accepted
                {
                    flatbuffers::FlatBufferBuilder builder;
                    auto acceptance = CreateSVConnectionStatus(builder,
                                                               ConnectionStatus_ACCEPTED);
                    auto message = CreateMessage(builder,
                                                 0,
                                                 Messages_SVConnectionStatus,
                                                 acceptance.Union());
                    builder.Finish(message);

                    SendSingle(builder, senderIp);

                        // And send him info about all players in a lobby
                    std::for_each(_playersConnections.cbegin(),
                                  _playersConnections.cend(),
                                  [&](auto& player)
                                  {
                                      auto nickname = builder.CreateString(playerConnection.GetName());
                                      auto connectionInfo = CreateSVPlayerConnected(builder,
                                                                                    playerConnection.GetUUID(),
                                                                                    nickname);
                                      auto message = CreateMessage(builder,
                                                                   0,
                                                                   Messages_SVPlayerConnected,
                                                                   connectionInfo.Union());
                                      builder.Finish(message);

                                      SendSingle(builder, senderIp);
                                  });
                }

                    // Notify everyone about new player
                _logger.Info() << "Player" << playerConnection.GetUUID() << " (" << playerConnection.GetName() << ") connected";
                {
                    flatbuffers::FlatBufferBuilder builder;
                    auto nickname = builder.CreateString(playerConnection.GetName());
                    auto connectionInfo = CreateSVPlayerConnected(builder,
                                                                  playerConnection.GetUUID(),
                                                                  nickname);
                    auto message = CreateMessage(builder,
                                                 0,
                                                 Messages_SVPlayerConnected,
                                                 connectionInfo.Union());
                    builder.Finish(message);

                    SendMulticast(builder);
                }

                break;
            }

            case GameMessage::Messages_CLPing:
            {
                auto ping = static_cast<const CLPing*>(message->payload());
                auto playerConnection = FindPlayerByUID(ping->player_uid());

                if(playerConnection != _playersConnections.end())
                    playerConnection->SetLastPacketTimepoint(Clock::now());

                break;
            }
            default:
                _logger.Warning() << "Unexpected event received in lobby_forming";
                break;
        }
        }

            // remove players with timeout and send notifications
        {
            std::lock_guard<std::recursive_mutex> lock(_playersMutex);
            _playersConnections.erase(
                                      std::remove_if(_playersConnections.begin(),
                                                     _playersConnections.end(),
                                                     [this](auto& playerConnection)
                                                     {
                                                         if(playerConnection.GetConnectionStatus() == PlayerConnection::ConnectionStatus::TIMEOUT)
                                                         {
                                                             _logger.Info() << "Player" << playerConnection.GetUUID() << " has been removed from server (connection timeout).";

                                                             flatbuffers::FlatBufferBuilder builder;
                                                             auto disconnect = CreateSVPlayerDisconnected(builder,
                                                                                                          playerConnection.GetLocalUID());
                                                             auto message = CreateMessage(builder,
                                                                                          0,
                                                                                          Messages_SVPlayerDisconnected,
                                                                                          disconnect.Union());
                                                             builder.Finish(message);
                                                             SendMulticast(builder);

                                                             return true;
                                                         }
                                                         return false;
                                                     }),
                                      _playersConnections.end());
        }

        if(_playersConnections.size() == _config.Players)
        {
            _state = GameServer::State::HERO_PICK;
            Task::setState(Poco::Task::TaskState::TASK_RUNNING);
            _logger.Info() << "STATE CHANGE: LOBBY-FORMING -> HERO-PICKING";

            flatbuffers::FlatBufferBuilder builder;
            auto pickStage = CreateSVHeroPickStage(builder);
            auto message = CreateMessage(builder,
                                         0,
                                         Messages_SVHeroPickStage,
                                         pickStage.Union());
            builder.Finish(message);

            SendMulticast(builder);
        }
    }
}

void GameServer::hero_picking_stage()
{
    using Player = std::pair<GameWorld::PlayerInfo, bool>;

    std::vector<Player> players;
    std::for_each(_playersConnections.cbegin(),
                  _playersConnections.cend(),
                  [&players](const PlayerConnection& playerConnection)
                  {
                      GameWorld::PlayerInfo info;
                      info.LocalUid = playerConnection.GetLocalUID();
                      info.Name = playerConnection.GetName();
                      info.Hero = Hero::Type::FIRST_HERO;
                      players.push_back(std::make_pair(info, false));
                  });

    while(_state == State::HERO_PICK)
    {
        std::this_thread::sleep_for(_msPerUpdate);

        while(_socket.available())
        {
            SafePacketGetter packetGetter(_socket);
            auto packet = packetGetter.Get<GameMessage::Message>();
            if(!packet)
                continue;

            auto message = GetMessage(packet->Data.data());
            auto playerConnection = FindPlayerByUID(message->sender_uid());

            if(playerConnection == _playersConnections.end())
            {
                _logger.Warning() << "Received packet from unexisting player";
                continue;
            }

            playerConnection->SetLastPacketTimepoint(Clock::now());

            switch(message->payload_type())
            {
            case Messages_CLHeroPick:
            {
                auto pick = static_cast<const CLHeroPick*>(message->payload());

                auto playerInfo = std::find_if(players.begin(),
                                               players.end(),
                                               [pick](const Player& info)
                                               {
                                                   return pick->player_uid() == info.first.LocalUid;
                                               });
                if(playerInfo == players.end())
                {
                    _logger.Warning() << "Player" << pick->player_uid() << " is not presented";
                    continue;
                }

                playerInfo->first.Hero = (Hero::Type)pick->hero_type();
                _logger.Info() << "Player" << playerInfo->first.LocalUid << " picked " << playerInfo->first.Hero;

                flatbuffers::FlatBufferBuilder builder;
                auto sv_pick = CreateSVHeroPick(builder,
                                                playerInfo->first.LocalUid,
                                                pick->hero_type());
                auto message = CreateMessage(builder,
                                             0,
                                             Messages_SVHeroPick,
                                             sv_pick.Union());
                builder.Finish(message);

                SendMulticast(builder);

                break;
            }

            case Messages_CLReadyToStart:
            {
                auto ready = static_cast<const CLReadyToStart*>(message->payload());

                auto playerInfo = std::find_if(players.begin(),
                                               players.end(),
                                               [ready](const Player& player)
                                               {
                                                   return ready->player_uid() == player.first.LocalUid;
                                               });
                if(playerInfo == players.end())
                {
                    _logger.Warning() << "Player" << ready->player_uid() << " is not presented";
                    continue;
                }

                    // TODO: remove after testing, or place in ifdef debug
                if(playerInfo->second == true)
                {
                    _logger.Warning() << "Player" << playerInfo->first.LocalUid << " sent ready packet more than once";
                }
                playerInfo->second = true;

                break;
            }

            case Messages_CLPing:
                break;

            default:
                _logger.Warning() << "Received unexpected event type in HERO-PICKING loop";
                break;
            }
        }

        {
            std::lock_guard<std::recursive_mutex> lock(_playersMutex);
            bool playerDisconnected = std::any_of(_playersConnections.cbegin(),
                                                  _playersConnections.cend(),
                                                  [](const PlayerConnection& playerConnection)
                                                  {
                                                      return playerConnection.GetConnectionStatus() == PlayerConnection::ConnectionStatus::TIMEOUT;
                                                  });

            if(playerDisconnected)
                throw std::runtime_error("Someone has disconnected during HERO-PICKING stage, feature with returning into LOBBY-FORMING is not yet implemented");
        }

            // check that players are ready to play
        bool everyoneReady = std::all_of(players.cbegin(),
                                         players.cend(),
                                         [](const Player& player)
                                         {
                                             return player.second;
                                         });

        if(everyoneReady)
        {
            _state = GameServer::State::GENERATING_WORLD;
            _logger.Info() << "STATE CHANGE: HERO-PICKING -> WORLD-GENERATION";

                // generate world for ourselves
            GameMapGenerator::Configuration mapConf;
            mapConf.Seed = _config.RandomSeed;
            mapConf.MapSize = 3;
            mapConf.RoomSize = 10;

            std::vector<GameWorld::PlayerInfo> playersInfo;
            std::for_each(players.cbegin(),
                          players.cend(),
                          [&playersInfo](const Player& player)
                          {
                              playersInfo.push_back(player.first);
                          });

                // if we throw from constructor - no reason to live anyway, GS will fall
            _world = std::make_unique<GameWorld>(mapConf,
                                                 playersInfo);

            flatbuffers::FlatBufferBuilder builder;
            auto generateMap = CreateSVGenerateMap(builder,
                                                   mapConf.MapSize,
                                                   mapConf.RoomSize,
                                                   mapConf.Seed);
            auto message = CreateMessage(builder,
                                         0,
                                         Messages_SVGenerateMap,
                                         generateMap.Union());
            builder.Finish(message);

            SendMulticast(builder);
        }
    }
}

void GameServer::world_generation_stage()
{
    using Player = std::pair<GameWorld::PlayerInfo, bool>;

    std::vector<Player> players;
    std::for_each(_playersConnections.cbegin(),
                  _playersConnections.cend(),
                  [&players](const PlayerConnection& playerConnection)
                  {
                      GameWorld::PlayerInfo info;
                      info.LocalUid = playerConnection.GetLocalUID();
                      info.Name = playerConnection.GetName();
                      info.Hero = Hero::Type::UNDEFINED;
                      players.push_back(std::make_pair(info, false));
                  });

    while(_state == State::GENERATING_WORLD)
    {
        std::this_thread::sleep_for(_msPerUpdate);

        while(_socket.available())
        {
            SafePacketGetter packetGetter(_socket);
            auto packet = packetGetter.Get<GameMessage::Message>();
            if(!packet)
                continue;

            auto message = GetMessage(packet->Data.data());
            auto playerConnection = FindPlayerByUID(message->sender_uid());

            if(playerConnection == _playersConnections.end())
            {
                _logger.Warning() << "Received packet from unexisting player";
                continue;
            }

            playerConnection->SetLastPacketTimepoint(Clock::now());

            switch(message->payload_type())
            {
            case Messages_CLMapGenerated:
            {
                auto generated = static_cast<const CLMapGenerated*>(message->payload());

                auto playerInfo = std::find_if(players.begin(),
                                               players.end(),
                                               [generated](const Player& info)
                                               {
                                                   return generated->player_uid() == info.first.LocalUid;
                                               });
                if(playerInfo == players.end())
                {
                    _logger.Warning() << "Player" << generated->player_uid() << " is not presented";
                    continue;
                }

                playerInfo->second = true;
                _logger.Info() << "Player" << playerInfo->first.LocalUid << " done world generation";

                break;
            }

            case Messages_CLPing:
                break;

            default:
                _logger.Warning() << "Received unexpected event type in HERO-PICKING loop";
                break;
            }
        }

        {
            std::lock_guard<std::recursive_mutex> lock(_playersMutex);
            bool playerDisconnected = std::any_of(_playersConnections.cbegin(),
                                                  _playersConnections.cend(),
                                                  [](const PlayerConnection& playerConnection)
                                                  {
                                                      return playerConnection.GetConnectionStatus() == PlayerConnection::ConnectionStatus::TIMEOUT;
                                                  });

            if(playerDisconnected)
                throw std::runtime_error("Someone has disconnected during WORLD-GENERATION stage, situation that can't be handled well");
        }
        
            // check that players are ready to play
        bool everyoneReady = std::all_of(players.cbegin(),
                                         players.cend(),
                                         [](const Player& player)
                                         {
                                             return player.second;
                                         });

        if(everyoneReady)
        {
            _logger.Info() << "STATE CHANGE: WORLD-GENERATION -> GAME-RUNNING";
            _state = State::RUNNING_GAME;

            flatbuffers::FlatBufferBuilder builder;
            auto start = CreateSVGameStart(builder);
            auto message = CreateMessage(builder,
                                         0,
                                         Messages_SVGameStart,
                                         start.Union());
            builder.Finish(message);

            SendMulticast(builder);
        }
    }
}

void GameServer::running_game_stage()
{
    ElapsedTime frameTime;

    while(_state == State::RUNNING_GAME)
    {
        frameTime.Reset();

        // sleep for some time, then get all packets and pass it to the gameworld, update
        std::this_thread::sleep_for(_msPerUpdate);

        if(!_socket.available())
        {
            auto anyoneActive = std::any_of(_playersConnections.cbegin(),
                                            _playersConnections.cend(),
                                            [](const PlayerConnection& playerConnection)
                                            {
                                                return playerConnection.GetConnectionStatus() == PlayerConnection::ConnectionStatus::ACTIVE;
                                            });

            if(!anyoneActive)
                throw std::runtime_error("No active connections with players, shutting down server (connections timeout)");
        }

        while(_socket.available())
        {
            SafePacketGetter packetGetter(_socket);
            auto packet = packetGetter.Get<GameMessage::Message>();
            if(!packet)
                continue;

            auto message = GetMessage(packet->Data.data());

            auto player = FindPlayerByUID(message->sender_uid());
            if(player == _playersConnections.end())
            {
                _logger.Warning() << "Received packet from unexisting player";
                continue;
            }

            player->SetLastPacketTimepoint(Clock::now());
            player->SetAddress(packet->Sender);

                // filter CLPing events
            if(message->payload_type() == Messages_CLPing)
                continue;

            _world->GetIncomingEvents().emplace(packet->Data.begin(),
                                                packet->Data.end());
        }

        auto& out_events = _world->GetOutgoingEvents();
        while(out_events.size())
        {
            SendMulticast(out_events.front());
            out_events.pop();
        }

        _world->update(frameTime.Elapsed<std::chrono::microseconds>());
    }
}

bool GameServer::PlayerExists(PlayerUID uid)
{
    return std::find_if(_playersConnections.cbegin(),
                        _playersConnections.cend(),
                        [uid](const PlayerConnection& playerConnection)
                        {
                            return playerConnection.GetUUID() == uid;
                        }) != _playersConnections.end();
}

std::vector<GameServer::PlayerConnection>::iterator GameServer::FindPlayerByUID(PlayerUID uid)
{
    std::lock_guard<std::recursive_mutex> lock(_playersMutex);
    return std::find_if(_playersConnections.begin(),
                        _playersConnections.end(),
                        [uid](const auto& player)
                        {
                            return player.GetUUID() == uid;
                        });
}
