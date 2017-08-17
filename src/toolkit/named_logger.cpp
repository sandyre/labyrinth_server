//
//  named_logger.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 06.07.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "named_logger.hpp"

#include "date.h"

#include <Poco/Thread.h>

#include <iomanip>
#include <iostream>


NamedLogger::LoggerStream::LoggerStream(const NamedLogger& parent,
                                        Level level)
: _logger(parent)
{
    switch(level)
    {
        case Level::DBG:
            _prefix += Color::RESET;
            _prefix += "[ Debug ] ";
            break;
        case Level::INFO:
            _prefix += Color::WHITE;
            _prefix += "[ Info ] ";
            break;
        case Level::WARNING:
            _prefix += Color::YELLOW;
            _prefix += "[ Warning ] ";
            break;
        case Level::ERROR:
            _prefix += Color::RED;
            _prefix += "[ Error ] ";
            break;
    }

    _stream << _prefix;
}


NamedLogger::NamedLogger(const std::string& name, Mode mode)
: _name(name),
  _mode(mode)
{
    if(_mode & Mode::FILE)
    {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);

        std::ostringstream oss;
        oss << _name << "";
        oss << std::put_time(&tm, "%d-%m-%Y-%H-%M-%S");
        oss << ".txt";

            // FIXME: no error handling
        _fileStream.open(oss.str());
    }
}

void
NamedLogger::Write(const std::string& str) const
{
    using namespace date;
    std::ostringstream oss;

    oss << Color::CYAN << "[ " << std::chrono::system_clock::now() << " ]";
    oss << Color::GREEN << "{ " << (Poco::Thread::current() ? Poco::Thread::current()->getName() : "__undefined__") << " }";
    oss << Color::MAGENTA << "[ " << _name << " ]";
    oss << str << std::endl;

    if(_mode & Mode::STDIO)
    {
        std::cout << oss.str();
    }
    if(_mode & Mode::FILE)
    {
        std::lock_guard<std::mutex> l(_fileMutex);
        _fileStream << oss.str();
        _fileStream.flush();
    }
}
