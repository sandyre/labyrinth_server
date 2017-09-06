//
//  MessageStorage.hpp
//  labyrinth_server
//
//  Created by Alexandr Borzykh on 07/09/2017.
//  Copyright © 2017 HATE|RED. All rights reserved.
//

#ifndef MessageStorage_hpp
#define MessageStorage_hpp

#include <boost/optional.hpp>

#include <deque>
#include <memory>
#include <mutex>

using Message = std::vector<uint8_t>;
using MessagePtr = std::shared_ptr<Message>;


class MessageStorage
{
    using Storage = std::deque<MessagePtr>;

public:
    MessageStorage(size_t messageLimit)
    { }

    void Push(const MessagePtr& message)
    {
        std::lock_guard<std::mutex> l(_mutex);
        _storage.push_back(message);
    }

    boost::optional<MessagePtr> Get(size_t index) const
    {
        std::lock_guard<std::mutex> l(_mutex);

        boost::optional<MessagePtr> message;

        if (index < _storage.size())
            message = _storage[index];

        return message;
    }

    size_t Size() const
    {
        std::lock_guard<std::mutex> l(_mutex);
        return _storage.size();
    }

private:
    mutable std::mutex  _mutex;
    Storage             _storage;
};
using MessageStoragePtr = std::shared_ptr<MessageStorage>;

#endif /* MessageStorage_hpp */
