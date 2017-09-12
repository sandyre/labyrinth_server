//
//  PlayerConnection.cpp
//  labyrinth_server
//
//  Created by Alexandr Borzykh on 05/09/2017.
//  Copyright © 2017 HATE|RED. All rights reserved.
//

#include "PlayerConnection.hpp"

#include "ConnectionsManager.hpp"

#include <thread>
#include <chrono>


PlayerConnection::PlayerConnection(ConnectionsManager& manager, Poco::Net::StreamSocket socket)
: _logger("PlayerConnection"),
  _socket(socket),
  _reactor(manager._reactor)
{
    _socket.setKeepAlive(true);

    // handshake protocol. receive UUID from client
    auto message = std::make_shared<MessageBuffer>(256, 0);
    _socket.receiveBytes(message->data(), message->size());

    _descriptor.Uuid = std::string(message->begin(), message->begin() + 36);

    _logger.Info() << "Established connection with player (identifier: " << _descriptor.Uuid << ")";
}


void PlayerConnection::onSocketReadable(const AutoPtr<ReadableNotification>& pNf)
{
    if (!_socket.available())
    {
        _logger.Warning() << "Connection closed by client";

        _reactor.removeEventHandler(_socket, NObserver<PlayerConnection, ReadableNotification>(*this, &PlayerConnection::onSocketReadable));
        _reactor.removeEventHandler(_socket, NObserver<PlayerConnection, WritableNotification>(*this, &PlayerConnection::onSocketWritable));

        _socket.close();

        _onDisconnect();
    }
    else if (_socket.available() >= 256)
    {
        auto message = std::make_shared<MessageBuffer>(MaxMessageSize);

        _socket.receiveBytes(message->data(), message->size());

        _onMessage(message);
    }
}


void PlayerConnection::onSocketWritable(const AutoPtr<WritableNotification>& pNf)
{
    std::lock_guard<std::mutex> l(_mutex);
    if (!_sendBuffer.empty())
    {
        const auto& message = _sendBuffer.front();

        try
        {
            _socket.sendBytes(message->data(), message->size());
            _sendBuffer.pop_front();

            _logger.Debug() << "Message sent";
        }
        catch (const std::exception& ex)
        { _logger.Error() << "Error in sending message: " << ex.what(); }
    }
    else
    {
        _reactor.removeEventHandler(_socket, NObserver<PlayerConnection, WritableNotification>(*this, &PlayerConnection::onSocketWritable));
    }
}


void PlayerConnection::SendMessage(const MessageBufferPtr& message)
{
    std::lock_guard<std::mutex> l(_mutex);

    if (message->size() > MaxMessageSize)
    {
        _logger.Warning() << "Constructed message size is MORE than MaxMessageSize, fix needed";
        return;
    }

    message->resize(256);
    _sendBuffer.push_back(message);

    if (!_reactor.hasEventHandler(_socket, NObserver<PlayerConnection, WritableNotification>(*this, &PlayerConnection::onSocketWritable)))
        _reactor.addEventHandler(_socket, NObserver<PlayerConnection, WritableNotification>(*this, &PlayerConnection::onSocketWritable));
}


boost::signals2::connection PlayerConnection::OnMessageConnector(const OnMessageSignal::slot_type& slot)
{
    auto connection = _onMessage.connect(slot);

    _reactor.addEventHandler(_socket, NObserver<PlayerConnection, ReadableNotification>(*this, &PlayerConnection::onSocketReadable));

    return connection;
}


boost::signals2::connection PlayerConnection::OnDisconnectConnector(const OnDisconnectSignal::slot_type& slot)
{ return _onDisconnect.connect(slot); }
