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
#include <random>

using namespace std::chrono;

using IPAddress = Poco::Net::SocketAddress;
using Clock = std::chrono::steady_clock;
using std::experimental::optional;


class GameServer::PlayerConnection
{
public:
    enum class ConnectionStatus
    {
        ACTIVE,
        TIMEOUT
    };

public:
    PlayerConnection(const std::string& uuid,
                     uint32_t localUuid,
                     const std::string& name)
    : _uuid(uuid),
      _localUUID(localUuid),
      _name(name),
      _timepoint(Clock::now())
    { }

    ConnectionStatus GetConnectionStatus() const
    {
        if(duration_cast<milliseconds>(Clock::now() - _timepoint) > 180s)
            return ConnectionStatus::TIMEOUT;

        return ConnectionStatus::ACTIVE;
    }

    const std::string& GetUUID() const
    { return _uuid; }

    uint32_t GetLocalUID() const
    { return _localUUID; }

    const std::string& GetName() const
    { return _name; }

    void SetLastPacketTimepoint(const Clock::time_point& timepoint)
    { _timepoint = timepoint; }

    const Clock::time_point& GetLastPacketTimepoint() const
    { return _timepoint; }

private:
    std::string         _uuid;
    uint32_t            _localUUID;
    std::string         _name;
    Clock::time_point   _timepoint;
};


const std::chrono::microseconds GameServer::PING_INTERVAL = 3s;


GameServer::GameServer(const Configuration& config)
: Task(("GameServer" + std::to_string(config.Port))),
  _state(GameServer::State::LOBBY_FORMING),
  _config(config),
  _msPerUpdate(10),
  _logger("Server", NamedLogger::Mode::STDIO)
{
    _logger.Info() << "Launch configuration {random_seed = " << _config.RandomSeed
            << ", lobby_size = " << _config.Players << ", refresh_rate = " << _msPerUpdate.count() << "ms}";


    try
    {
        _connectionsManager = std::make_shared<ConnectionsManager>(config.Port);

        // subscribe for all TCP messages
        _connectionManagerConnection = _connectionsManager->OnMessageConnector(std::bind(&GameServer::MessageHandler, this, std::placeholders::_1));
    }
    catch(const std::exception& e)
    {
        _logger.Error() << "Failed to bind socket to port, exception thrown: " << e.what();
        shutdown();
    }

    Task::setState(Poco::Task::TaskState::TASK_IDLE);
}


void GameServer::shutdown()
{ Task::setState(Poco::Task::TaskState::TASK_FINISHED); }


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


void GameServer::lobby_forming_stage()
{
    RandomGenerator<std::mt19937, std::uniform_real_distribution<>> randGen(5000, 30000, 5); // FIXME: random seed?

    while(_state == State::LOBBY_FORMING)
    {
        std::this_thread::sleep_for(_msPerUpdate);

        MessageStorage messages;
        {
            std::lock_guard<std::mutex> l(_mutex);
            messages.swap(_messages);
        }

        for (const auto& messageBuf : messages)
        {
            if(!flatbuffers::Verifier(messageBuf->data(), messageBuf->size()).VerifyBuffer<GameMessage::Message>(nullptr))
            {
                _logger.Warning() << "Packet verification failed, skip";
                return;
            }
            
            auto message = GameMessage::GetMessage(messageBuf->data());

            switch(message->payload_type())
            {
            case GameMessage::Messages_CLConnection:
            {
                auto con_info = static_cast<const GameMessage::CLConnection *>(message->payload());

                if(PlayerExists(message->sender_uid()->c_str()))
                {
                    _logger.Warning() << "Player, which has already been added into lobby, tried to connect twice";
                    continue;
                }

                    // FIXME: should make Builder for PlayerConnection to validate input
                PlayerConnection playerConnection(message->sender_uid()->c_str(),
                                                  randGen.NextInt(),
                                                  std::string(con_info->nickname()->c_str()));
                _playersConnections.push_back(playerConnection);

                    // Notify player that he is accepted
                {
                    flatbuffers::FlatBufferBuilder builder;
                    auto acceptance = GameMessage::CreateSVConnectionStatus(builder,
                                                                            playerConnection.GetLocalUID(),
                                                                            GameMessage::ConnectionStatus_ACCEPTED);
                    auto message = GameMessage::CreateMessage(builder,
                                                              0,
                                                              GameMessage::Messages_SVConnectionStatus,
                                                              acceptance.Union());
                    builder.Finish(message);

                    _connectionsManager->Multicast(std::make_shared<MessageBuffer>(builder.GetBufferPointer(),
                                                                                   builder.GetBufferPointer() + builder.GetSize()));

                        // And send him info about all players in a lobby
                    std::for_each(_playersConnections.cbegin(),
                                  _playersConnections.cend(),
                                  [&](auto& player)
                                  {
                                      flatbuffers::FlatBufferBuilder builder;
                                      auto nickname = builder.CreateString(player.GetName());
                                      auto connectionInfo = GameMessage::CreateSVPlayerConnected(builder,
                                                                                                 player.GetLocalUID(),
                                                                                                 nickname);
                                      auto message = CreateMessage(builder,
                                                                   0,
                                                                   GameMessage::Messages_SVPlayerConnected,
                                                                   connectionInfo.Union());
                                      builder.Finish(message);

                                      _connectionsManager->Multicast(std::make_shared<MessageBuffer>(builder.GetBufferPointer(),
                                                                                                     builder.GetBufferPointer() + builder.GetSize()));
                                  });
                }

                    // Notify everyone about new player
                _logger.Info() << "Player [UUID:" << playerConnection.GetUUID() << "] LocalUID: [" << playerConnection.GetLocalUID() << "] Nickname: [" << playerConnection.GetName() <<  "] connected";
                {
                    flatbuffers::FlatBufferBuilder builder;
                    auto nickname = builder.CreateString(playerConnection.GetName());
                    auto connectionInfo = GameMessage::CreateSVPlayerConnected(builder,
                                                                  playerConnection.GetLocalUID(),
                                                                  nickname);
                    auto message = CreateMessage(builder,
                                                 0,
                                                 GameMessage::Messages_SVPlayerConnected,
                                                 connectionInfo.Union());
                    builder.Finish(message);

                    _connectionsManager->Multicast(std::make_shared<MessageBuffer>(builder.GetBufferPointer(),
                                                                                   builder.GetBufferPointer() + builder.GetSize()));
                }

                break;
            }

            default:
                _logger.Warning() << "Unexpected event received in lobby_forming";
                break;
        }
        }

            // remove players with timeout and send notifications
        _playersConnections.erase(
                                  std::remove_if(_playersConnections.begin(),
                                                 _playersConnections.end(),
                                                 [this](auto& playerConnection)
                                                 {
                                                     if(playerConnection.GetConnectionStatus() == PlayerConnection::ConnectionStatus::TIMEOUT)
                                                     {
                                                         _logger.Info() << "Player" << playerConnection.GetUUID() << " has been removed from server (connection timeout).";

                                                         flatbuffers::FlatBufferBuilder builder;
                                                         auto disconnect = GameMessage::CreateSVPlayerDisconnected(builder,
                                                                                                                   playerConnection.GetLocalUID());
                                                         auto message = GameMessage::CreateMessage(builder,
                                                                                                   0,
                                                                                                   GameMessage::Messages_SVPlayerDisconnected,
                                                                                                   disconnect.Union());
                                                         builder.Finish(message);

                                                         _connectionsManager->Multicast(std::make_shared<MessageBuffer>(builder.GetBufferPointer(),
                                                                                                                        builder.GetBufferPointer() + builder.GetSize()));

                                                         return true;
                                                     }
                                                     return false;
                                                 }),
                                  _playersConnections.end());

        if(_playersConnections.size() == _config.Players)
        {
            _state = GameServer::State::HERO_PICK;
            Task::setState(Poco::Task::TaskState::TASK_RUNNING);
            _logger.Info() << "STATE CHANGE: LOBBY-FORMING -> HERO-PICKING";

            flatbuffers::FlatBufferBuilder builder;
            auto pickStage = GameMessage::CreateSVHeroPickStage(builder);
            auto message = GameMessage::CreateMessage(builder,
                                                      0,
                                                      GameMessage::Messages_SVHeroPickStage,
                                                      pickStage.Union());
            builder.Finish(message);

            _connectionsManager->Multicast(std::make_shared<MessageBuffer>(builder.GetBufferPointer(),
                                                                           builder.GetBufferPointer() + builder.GetSize()));
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

        MessageStorage messages;
        {
            std::lock_guard<std::mutex> l(_mutex);
            messages.swap(_messages);
        }

        for (const auto& messageBuf : messages)
        {
            if(!flatbuffers::Verifier(messageBuf->data(), messageBuf->size()).VerifyBuffer<GameMessage::Message>(nullptr))
            {
                _logger.Warning() << "Packet verification failed, skip";
                return;
            }

            auto message = GameMessage::GetMessage(messageBuf->data());
            auto playerConnection = FindPlayerByUID(message->sender_uid()->c_str());

            if(playerConnection == _playersConnections.end())
            {
                _logger.Warning() << "Received packet from unexisting player";
                continue;
            }

            playerConnection->SetLastPacketTimepoint(Clock::now());

            switch(message->payload_type())
            {
            case GameMessage::Messages_CLHeroPick:
            {
                auto pick = static_cast<const GameMessage::CLHeroPick*>(message->payload());

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
                auto sv_pick = GameMessage::CreateSVHeroPick(builder,
                                                             playerInfo->first.LocalUid,
                                                             pick->hero_type());
                auto message = GameMessage::CreateMessage(builder,
                                                          0,
                                                          GameMessage::Messages_SVHeroPick,
                                                          sv_pick.Union());
                builder.Finish(message);

                _connectionsManager->Multicast(std::make_shared<MessageBuffer>(builder.GetBufferPointer(),
                                                                               builder.GetBufferPointer() + builder.GetSize()));

                break;
            }

            case GameMessage::Messages_CLReadyToStart:
            {
                auto ready = static_cast<const GameMessage::CLReadyToStart*>(message->payload());

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

            case GameMessage::Messages_CLPing:
                break;

            default:
                _logger.Warning() << "Received unexpected event type in HERO-PICKING loop";
                break;
            }
        }

        bool playerDisconnected = std::any_of(_playersConnections.cbegin(),
                                              _playersConnections.cend(),
                                              [](const PlayerConnection& playerConnection)
                                              {
                                                  return playerConnection.GetConnectionStatus() == PlayerConnection::ConnectionStatus::TIMEOUT;
                                              });

        if(playerDisconnected)
            throw std::runtime_error("Someone has disconnected during HERO-PICKING stage, feature with returning into LOBBY-FORMING is not yet implemented");

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

                // Log UUID to LocaUID mapping
            _logger.Info() << "Players UUID <-> LocalUID mapping";
            for(auto& player : _playersConnections)
                _logger.Info() << player.GetUUID() << " -> " << player.GetLocalUID();

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
            auto generateMap = GameMessage::CreateSVGenerateMap(builder,
                                                                mapConf.MapSize,
                                                                mapConf.RoomSize,
                                                                mapConf.Seed);
            auto message = GameMessage::CreateMessage(builder,
                                                      0,
                                                      GameMessage::Messages_SVGenerateMap,
                                                      generateMap.Union());
            builder.Finish(message);

            _connectionsManager->Multicast(std::make_shared<MessageBuffer>(builder.GetBufferPointer(),
                                                                           builder.GetBufferPointer() + builder.GetSize()));
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

        MessageStorage messages;
        {
            std::lock_guard<std::mutex> l(_mutex);
            messages.swap(_messages);
        }

        for (const auto& messageBuf : messages)
        {
            if(!flatbuffers::Verifier(messageBuf->data(), messageBuf->size()).VerifyBuffer<GameMessage::Message>(nullptr))
            {
                _logger.Warning() << "Packet verification failed, skip";
                continue;
            }

            auto message = GameMessage::GetMessage(messageBuf->data());
            auto playerConnection = FindPlayerByUID(message->sender_uid()->c_str());

            if(playerConnection == _playersConnections.end())
            {
                _logger.Warning() << "Received packet from unexisting player";
                continue;
            }

            playerConnection->SetLastPacketTimepoint(Clock::now());

            switch(message->payload_type())
            {
            case GameMessage::Messages_CLMapGenerated:
            {
                auto generated = static_cast<const GameMessage::CLMapGenerated*>(message->payload());

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

            default:
                _logger.Warning() << "Received unexpected event type in HERO-PICKING loop";
                break;
            }
        }

        bool playerDisconnected = std::any_of(_playersConnections.cbegin(),
                                              _playersConnections.cend(),
                                              [](const PlayerConnection& playerConnection)
                                              {
                                                  return playerConnection.GetConnectionStatus() == PlayerConnection::ConnectionStatus::TIMEOUT;
                                              });

        if(playerDisconnected)
            throw std::runtime_error("Someone has disconnected during WORLD-GENERATION stage, situation that can't be handled well");
        
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
            auto start = GameMessage::CreateSVGameStart(builder);
            auto message = CreateMessage(builder,
                                         0,
                                         GameMessage::Messages_SVGameStart,
                                         start.Union());
            builder.Finish(message);

            _connectionsManager->Multicast(std::make_shared<MessageBuffer>(builder.GetBufferPointer(),
                                                                           builder.GetBufferPointer() + builder.GetSize()));
        }
    }
}


void GameServer::running_game_stage()
{
    ElapsedTime frameTime;

    while(_world->GetState() != GameWorld::State::FINISHED)
    {
        frameTime.Reset();

        // sleep for some time, then get all packets and pass it to the gameworld, update
        std::this_thread::sleep_for(_msPerUpdate);

        MessageStorage messages;
        {
            std::lock_guard<std::mutex> l(_mutex);
            messages.swap(_messages);
        }

        _world->update(messages, frameTime.Elapsed<std::chrono::microseconds>());

        messages.swap(_world->GetOutgoingMessages());
        for (const auto& message : messages)
            _connectionsManager->Multicast(message);
    }
}


bool GameServer::PlayerExists(const std::string& uid)
{
    return std::find_if(_playersConnections.cbegin(),
                        _playersConnections.cend(),
                        [uid](const PlayerConnection& playerConnection)
                        {
                            return playerConnection.GetUUID() == uid;
                        }) != _playersConnections.end();
}


std::vector<GameServer::PlayerConnection>::iterator GameServer::FindPlayerByUID(const std::string& uid)
{
    return std::find_if(_playersConnections.begin(),
                        _playersConnections.end(),
                        [uid](const auto& player)
                        {
                            return player.GetUUID() == uid;
                        });
}
