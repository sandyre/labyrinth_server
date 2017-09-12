//
//  ConnectionsManager.hpp
//  labyrinth_server
//
//  Created by Alexandr Borzykh on 30/08/2017.
//  Copyright © 2017 HATE|RED. All rights reserved.
//

#ifndef ConnectionsManager_hpp
#define ConnectionsManager_hpp

#include "Message.hpp"
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
#include <mutex>
#include <vector>


class ConnectionsManager
    : public Poco::Runnable
{
    friend PlayerConnection;

    using Socket = Poco::Net::StreamSocket;
    using Connection = std::pair<boost::signals2::connection, PlayerConnectionPtr>;
    using Connections = std::deque<Connection>;

    using OnMessageSignal = boost::signals2::signal<void(const MessageBufferPtr&)>;

public:
    ConnectionsManager(uint16_t listeningPort, size_t maxConnections = 4);
    ~ConnectionsManager();

    boost::signals2::connection OnMessageConnector(const OnMessageSignal::slot_type& slot)
    { return _onMessage.connect(slot); }

    void Multicast(const MessageBufferPtr& message)
    {
        std::lock_guard<std::mutex> l(_mutex);
        for (const auto& connection : _connections)
            connection.second->SendMessage(message);
    }

private:
    virtual void run() override;

    void DisconnectHandler();
    void MessageHandler(const MessageBufferPtr& message);

private:
    NamedLogger                     _logger;
    size_t                          _maxConnections;
    Poco::Net::ServerSocket         _socket;

    std::atomic_bool                _alive;
    Poco::Thread                    _worker;

    Poco::Net::SocketReactor        _reactor;
    Poco::Thread                    _reactorWorker;

    std::mutex                      _mutex;
    Connections                     _connections;

    OnMessageSignal                 _onMessage;
};
using ConnectionsManagerPtr = std::shared_ptr<ConnectionsManager>;

#endif /* ConnectionsManager_hpp */
