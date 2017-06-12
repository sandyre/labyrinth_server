//
//  shell.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 11.06.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef shell_hpp
#define shell_hpp

#include "logsystem.hpp"

#include <Poco/Runnable.h>

class MasterServer;

class Shell : public Poco::Runnable
{
public:
    Shell(MasterServer&);
    
    void init();
    void run();
protected:
    MasterServer& m_oMasterServer;
    
        // Logging
    LogSystem m_oLogSys;
    std::ostringstream m_oMsgBuilder;
};

#endif /* shell_hpp */
