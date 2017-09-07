//
//  ConnectionsManager.hpp
//  labyrinth_server
//
//  Created by Alexandr Borzykh on 30/08/2017.
//  Copyright © 2017 HATE|RED. All rights reserved.
//

#ifndef ConnectionsManager_hpp
#define ConnectionsManager_hpp

#include "MessageStorage.hpp"
#include "PlayerConnection.hpp"
#include "../toolkit/named_logger.hpp"

#include <boost/signals2.hpp>

#include <Poco/Runnable.h>
#include <Poco/NObserver.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/SocketReactor.h>
#include <Poco/Net/StreamSocket.h>

#include <atomic>
#include <deque>
#include <memory>
#include <vector>

using Message = std::vector<uint8_t>;
using MessagePtr = std::shared_ptr<Message>;
using MessageRecord = std::pair<PlayerConnection::Descriptor, const MessagePtr&>;

class ConnectionsManager
    : public Poco::Runnable
{
    using Connection = Poco::Net::StreamSocket;

public:
    ConnectionsManager(uint16_t listeningPort, size_t maxConnections = 4);
    ~ConnectionsManager();

    boost::signals2::signal<void(const MessagePtr&)>& OnMessage()
    { return _onMessage; }

private:
    virtual void run() override;

    void MessageHandler(const MessagePtr& message);

private:
    NamedLogger                     _logger;
    size_t                          _maxConnections;
    Poco::Net::ServerSocket         _socket;

    std::atomic_bool                _alive;
    Poco::Thread                    _worker;

    Poco::Net::SocketReactor        _reactor;
    Poco::Thread                    _reactorWorker;

    std::deque<PlayerConnectionPtr> _connections;

    boost::signals2::signal<void(Connection)>        _onConnectionAccepted;
    boost::signals2::signal<void(const MessagePtr&)> _onMessage;
};

#endif /* ConnectionsManager_hpp */
