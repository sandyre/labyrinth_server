//
//  Message.hpp
//  labyrinth_server
//
//  Created by Alexandr Borzykh on 09/09/2017.
//  Copyright © 2017 HATE|RED. All rights reserved.
//

#ifndef Message_h
#define Message_h

#include <deque>
#include <memory>
#include <vector>

using MessageBuffer = std::vector<uint8_t>;
using MessageBufferPtr = std::shared_ptr<MessageBuffer>;

using MessageStorage = std::deque<MessageBufferPtr>;

#endif /* Message_h */
