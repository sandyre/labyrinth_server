//
//  SafePacketGetter.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 11.07.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef SafePacketGetter_hpp
#define SafePacketGetter_hpp

#include "named_logger.hpp"
#include "optional.hpp"

#include <Poco/Net/DatagramSocket.h>
#include <flatbuffers/flatbuffers.h>

#include <array>

struct Packet
{
    Poco::Net::SocketAddress    Sender;
    std::vector<uint8_t>        Data;
};

class SafePacketGetter
{
public:
    SafePacketGetter(Poco::Net::DatagramSocket& socket)
    : _logger("SafePacketGetter", NamedLogger::Mode::STDIO),
      _socket(socket)
    { }

    template<typename T>
    std::experimental::optional<Packet> Get()
    {
        Packet packet;

        if(_socket.available() > _internalBuffer.size())
        {
            _socket.receiveFrom(_internalBuffer.data(),
                                1,
                                packet.Sender);

            _logger.Warning() << "Received packet which size is more than buffer_size. Probably, its a hack or DDoS. Sender addr: " << packet.Sender.toString();

            return std::experimental::optional<Packet>();
        }

        auto packSize = _socket.receiveFrom(_internalBuffer.data(),
                                            _internalBuffer.size(),
                                            packet.Sender);

        flatbuffers::Verifier verifier(_internalBuffer.data(),
                                       packSize);
        if(!verifier.VerifyBuffer<T>(nullptr))
        {
            _logger.Warning() << "Packet verification failed, probably a DDoS. Sender addr: " << packet.Sender.toString();

            return std::experimental::optional<Packet>();
        }

        packet.Data.assign(_internalBuffer.begin(),
                           _internalBuffer.begin() + packSize);
        return packet;
    }

private:
    NamedLogger                 _logger;
    Poco::Net::DatagramSocket&  _socket;
    std::array<uint8_t, 4096>   _internalBuffer;
};

#endif /* SafePacketGetter_hpp */
