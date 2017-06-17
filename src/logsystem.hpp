//
//  logsystem.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 24.02.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef logsystem_hpp
#define logsystem_hpp

#include <string>
#include <sstream>
#include <fstream>
#include <ostream>

const std::string red("\033[0;31m");
const std::string green("\033[1;32m");
const std::string yellow("\033[1;33m");
const std::string blue("\033[1;34m");
const std::string cyan("\033[0;36m");
const std::string magenta("\033[0;35m");
const std::string reset("\033[0m");

class LogSystem
{
public:
    enum Mode
    {
        FILE  = 0x01,
        STDIO = 0x02,
        MIXED = 0x03
    };
public:
    LogSystem();
    ~LogSystem();

    void Init(const std::string& service_name, Mode mode);
    void Close();

    void Info(const std::string& msg);
    void Warning(const std::string& msg);
    void Error(const std::string& msg);
private:
    Mode          _mode;
    std::string   _serviceName;
    std::ofstream _fileStream;

    std::istringstream m_oWarningStream;
    std::istringstream m_oErrorStream;
    std::istringstream m_oInfoStream;
};

#endif /* logsystem_hpp */
