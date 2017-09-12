//
//  ConnectionsManager.cpp
//  labyrinth_server
//
//  Created by Alexandr Borzykh on 30/08/2017.
//  Copyright © 2017 HATE|RED. All rights reserved.
//

#include "ConnectionsManager.hpp"

#include <Poco/RunnableAdapter.h>

using Poco::NObserver;


ConnectionsManager::ConnectionsManager(uint16_t listeningPort, size_t maxConnections)
: _logger("ConnectionsManager"),
  _maxConnections(maxConnections),
  _socket(Poco::Net::SocketAddress(Poco::Net::IPAddress(), listeningPort)),
  _alive(true)
{
    _worker.setName("ConnectionsManager");
    _worker.start(*this);

    _reactor.setTimeout(Poco::Timespan(0, 25));
    _reactorWorker.setName("ConnectionsReactor");
    _reactorWorker.start(_reactor);
}


ConnectionsManager::~ConnectionsManager()
{
    _reactor.stop();
    _alive = false;
    _worker.join();
}


void ConnectionsManager::run()
{
    while (_alive)
    {
        if (_socket.poll(Poco::Timespan(0, 25000), Poco::Net::Socket::SELECT_READ)
            && _connections.size() < _maxConnections)
        {
            Socket connection = _socket.acceptConnection();

            _logger.Info() << "Requested connection from " << connection.peerAddress().toString();
            
            // TODO: do validation
            try
            {
                auto playerConnection = std::make_shared<PlayerConnection>(*this, connection);
                playerConnection->OnDisconnectConnector(std::bind(&ConnectionsManager::DisconnectHandler, this));
                auto signal_connection = playerConnection->OnMessageConnector(std::bind(&ConnectionsManager::MessageHandler, this, std::placeholders::_1));

                _connections.emplace_back(signal_connection, playerConnection);
            }
            catch (const std::exception& ex)
            { _logger.Error() << "Error occured in new connection establishment: " << ex.what(); }
        }
    }
}


void ConnectionsManager::MessageHandler(const MessageBufferPtr& message)
{
    _logger.Debug() << "Forwarding message";

    _onMessage(message);
}


void ConnectionsManager::DisconnectHandler()
{
    _logger.Debug() << "Player disconnected, shutdown reactor";

    _reactor.stop();
}
