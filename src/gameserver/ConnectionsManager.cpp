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
        if (_socket.poll(Poco::Timespan(0, 500), Poco::Net::Socket::SELECT_READ)
            && _connections.size() < _maxConnections)
        {
            Connection connection = _socket.acceptConnection();

            _logger.Info() << "Requested connection from " << connection.peerAddress().toString();
            
            // TODO: do validation
            auto playerConnection = std::make_shared<PlayerConnection>(connection, _reactor);
            playerConnection->OnMessage().connect(std::bind(&ConnectionsManager::MessageHandler, this, std::placeholders::_1));
            _connections.push_back(playerConnection);
        }
    }
}


void ConnectionsManager::MessageHandler(const MessagePtr& message)
{
    _logger.Debug() << "Forwarding message";

    _onMessage(message);
}
