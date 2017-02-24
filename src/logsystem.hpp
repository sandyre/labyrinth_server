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
#include <fstream>

class LogSystem
{
public:
    enum Mode
    {
        FILE = 0x01,
        STDIO = 0x02,
        MIXED = 0x03
    };
public:
    LogSystem();
    ~LogSystem();
    
    void    Init(const std::string& service_name, Mode mode);
    void    Write(const std::string& msg);
    void    Close();
private:
    Mode          m_eMode;
    std::string   m_oServiceName;
    std::ofstream m_oFileStream;
};

#endif /* logsystem_hpp */
