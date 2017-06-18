//
//  logsystem.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 24.02.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "logsystem.hpp"
#include "date.h"

#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>

#include <Poco/Thread.h>

LogSystem::LogSystem()
{
    
}

LogSystem::~LogSystem()
{
    if(_fileStream.is_open())
        _fileStream.close();
}

void
LogSystem::Init(const std::string &service_name,
                Mode mode)
{
    _mode = mode;
    _serviceName = service_name;
    
    if(mode & Mode::FILE) // both FILE and MIXED catched
    {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        
        std::ostringstream oss;
        oss << service_name << "";
        oss << std::put_time(&tm, "%d-%m-%Y-%H-%M-%S");
        oss << ".txt";
        
            // FIXME: no error handling
        _fileStream.open(oss.str());
    }
}

void
LogSystem::Info(const std::string& msg)
{
    using namespace date;
    std::ostringstream oss;
    oss << cyan;
    oss << "[" << std::chrono::system_clock::now() << "]";
    oss << green;
    oss << "{ " << (Poco::Thread::current() ? Poco::Thread::current()->getName() : "_undefined_") << " }";
    oss << magenta;
    oss << "[ " << _serviceName << " ]";
    oss << white;
    oss << "[ Info ] ";
    oss << msg << "\n";
    oss << reset;
    
    if(_mode & Mode::FILE)
    {
        _fileStream << oss.str();
        _fileStream.flush();
    }
    if(_mode & Mode::STDIO)
    {
        std::cout << oss.str();
    }
}

void
LogSystem::Warning(const std::string& msg)
{
    using namespace date;
    std::ostringstream oss;
    oss << cyan;
    oss << "[" << std::chrono::system_clock::now() << "]";
    oss << green;
    oss << "{ " << (Poco::Thread::current() ? Poco::Thread::current()->getName() : "_undefined_") << " }";
    oss << magenta;
    oss << "[ " << _serviceName << " ]";
    oss << yellow;
    oss << "[ Warning ] ";
    oss << msg << "\n";
    oss << reset;
    
    if(_mode & Mode::FILE)
    {
        _fileStream << oss.str();
        _fileStream.flush();
    }
    if(_mode & Mode::STDIO)
    {
        std::cout << oss.str();
    }
}

void
LogSystem::Error(const std::string& msg)
{
    using namespace date;
    std::ostringstream oss;
    oss << cyan;
    oss << "[" << std::chrono::system_clock::now() << "]";
    oss << green;
    oss << "{ " << (Poco::Thread::current() ? Poco::Thread::current()->getName() : "_undefined_") << " }";
    oss << magenta;
    oss << "[ " << _serviceName << " ]";
    oss << red;
    oss << "[ Error ] ";
    oss << msg << "\n";
    oss << reset;
    
    if(_mode & Mode::FILE)
    {
        _fileStream << oss.str();
        _fileStream.flush();
    }
    if(_mode & Mode::STDIO)
    {
        std::cout << oss.str();
    }
}

void
LogSystem::Close()
{
    _fileStream.close();
}
