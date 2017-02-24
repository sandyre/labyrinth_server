//
//  logsystem.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 24.02.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "logsystem.hpp"

#include <ctime>
#include <sstream>
#include <iomanip>
#include <iostream>

LogSystem::LogSystem()
{
    
}

LogSystem::~LogSystem()
{
    if(m_oFileStream.is_open())
        m_oFileStream.close();
}

void
LogSystem::Init(const std::string &service_name,
                Mode mode)
{
    m_eMode = mode;
    m_oServiceName = service_name;
    
    if(mode & Mode::FILE) // both FILE and MIXED catched
    {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        
        std::ostringstream oss;
        oss << service_name << "";
        oss << std::put_time(&tm, "%d-%m-%Y-%H-%M-%S");
        oss << ".txt";
        
            // FIXME: no error handling
        m_oFileStream.open(oss.str());
    }
}

void
LogSystem::Write(const std::string &msg)
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "[%H-%M-%S]");
    oss << "[" << m_oServiceName << "] ";
    oss << msg << "\n";
    
    if(m_eMode & Mode::FILE)
    {
        m_oFileStream << oss.str();
        m_oFileStream.flush();
    }
    if(m_eMode & Mode::STDIO)
    {
        std::cout << oss.str();
    }
}

void
LogSystem::Close()
{
    m_oFileStream.close();
}
