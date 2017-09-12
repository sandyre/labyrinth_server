//
//  PlayerConnection.hpp
//  labyrinth_server
//
//  Purpose: this class stores information about player connection, handles opened socket,
//           stores messages to-be-send
//
//  Created by Alexandr Borzykh on 05/09/2017.
//  Copyright © 2017 HATE|RED. All rights reserved.
//

#ifndef PlayerConnection_hpp
#define PlayerConnection_hpp

#include "Message.hpp"
#include "../toolkit/named_logger.hpp"

#include <boost/signals2.hpp>

#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/SocketNotification.h>
#include <Poco/Net/SocketReactor.h>
#include <Poco/NObserver.h>

#include <deque>
#include <memory>
#include <mutex>

using Poco::AutoPtr;
using Poco::NObserver;
using Poco::Net::ReadableNotification;
using Poco::Net::WritableNotification;
using Poco::Net::ShutdownNotification;
using Poco::Net::ErrorNotification;
using Poco::Net::TimeoutNotification;

class ConnectionsManager;


class PlayerConnection
{
    const size_t MaxMessageSize = 1024;

    using OnMessageSignal = boost::signals2::signal<void(const MessageBufferPtr&)>;
    using OnDisconnectSignal = boost::signals2::signal<void()>;
    
public:
    struct Descriptor
    {
        std::string Uuid;
    };

public:
    PlayerConnection(ConnectionsManager& manager, Poco::Net::StreamSocket socket);

    Descriptor GetDescriptor() const { return _descriptor; }

    void SendMessage(const MessageBufferPtr& message);

    boost::signals2::connection OnDisconnectConnector(const OnDisconnectSignal::slot_type& slot);
    boost::signals2::connection OnMessageConnector(const OnMessageSignal::slot_type& slot);

private:
    void onSocketReadable(const AutoPtr<ReadableNotification>& pNf);

    void onSocketWritable(const AutoPtr<WritableNotification>& pNf);

private:
    NamedLogger                 _logger;
    Descriptor                  _descriptor;
    Poco::Net::StreamSocket     _socket;
    Poco::Net::SocketReactor&   _reactor;

    std::mutex                  _mutex;
    MessageStorage              _sendBuffer;

    OnDisconnectSignal          _onDisconnect;
    OnMessageSignal             _onMessage;
};
using PlayerConnectionPtr = std::shared_ptr<PlayerConnection>;

#endif /* PlayerConnection_hpp */
