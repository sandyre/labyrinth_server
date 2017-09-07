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

using Message = std::vector<uint8_t>;
using MessagePtr = std::shared_ptr<Message>;


class PlayerConnection
{
    const size_t MaxMessageSize = 1024;

public:
    struct Descriptor
    {
        std::string Uuid;
    };

public:
    PlayerConnection(Poco::Net::StreamSocket socket, Poco::Net::SocketReactor& reactor)
    : _logger("PlayerConnection"),
      _socket(socket),
      _reactor(reactor)
    {
        _socket.setKeepAlive(true);

        _reactor.addEventHandler(_socket, NObserver<PlayerConnection, ReadableNotification>(*this, &PlayerConnection::onSocketReadable));
        _reactor.addEventHandler(_socket, NObserver<PlayerConnection, ErrorNotification>(*this, &PlayerConnection::onSocketError));
    }

    void onSocketReadable(const AutoPtr<ReadableNotification>& pNf)
    {
        if (!_socket.available())
        {
            _logger.Warning() << "Connection closed by client";

            _reactor.removeEventHandler(_socket, NObserver<PlayerConnection, ReadableNotification>(*this, &PlayerConnection::onSocketReadable));
            _reactor.removeEventHandler(_socket, NObserver<PlayerConnection, ErrorNotification>(*this, &PlayerConnection::onSocketError));

            _socket.close();
        }
        else
        {
            auto message = std::make_shared<Message>(MaxMessageSize, 0);

            _socket.receiveBytes(message->data(), message->size());

            _onMessage(message);
        }
    }

    void onSocketWritable(const AutoPtr<WritableNotification>& pNf)
    {
        _logger.Info() << "Connection writable";
    }

    void onSocketError(const AutoPtr<ErrorNotification>& pNf)
    {
        _logger.Info() << "Socket error";
    }

    void ResetSocket(Poco::Net::StreamSocket socket)
    { _socket = socket; }

    Descriptor GetDescriptor() const
    { return _descriptor; }

    Poco::Net::Socket GetSocket() const
    { return _socket; }

    boost::signals2::signal<void(const MessagePtr&)>& OnMessage()
    { return _onMessage; }

private:
    NamedLogger                 _logger;
    Descriptor                  _descriptor;
    Poco::Net::StreamSocket     _socket;
    Poco::Net::SocketReactor&   _reactor;

    boost::signals2::signal<void(const MessagePtr&)> _onMessage;
};
using PlayerConnectionPtr = std::shared_ptr<PlayerConnection>;

#endif /* PlayerConnection_hpp */
