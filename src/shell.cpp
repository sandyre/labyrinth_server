//
//  shell.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 11.06.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "shell.hpp"

#include "masterserver.hpp"

#include <iostream>

Shell::Shell(MasterServer& ms) :
m_oMasterServer(ms)
{
    
}

void
Shell::init()
{
        // Init logging
    m_oLogSys.Init("Shell", LogSystem::Mode::STDIO);
}

void
Shell::run()
{
    std::string command;
    while(std::cin >> command &&
          command != "halt")
    {
        if(command == "status")
        {
            m_oMsgBuilder << "System status";
            m_oMsgBuilder << "\nPlayers in queue: " << m_oMasterServer._playersPool.size();
            m_oMsgBuilder << "\nServers online: " << m_oMasterServer._gameServers.size();
            m_oLogSys.Info(m_oMsgBuilder.str());
            m_oMsgBuilder.str("");
        }
        else if(command == "list-servers")
        {
            m_oMsgBuilder << "Game servers online: " << m_oMasterServer._gameServers.size() << "\n";
            for(auto& server : m_oMasterServer._gameServers)
            {
                    // TODO: bullshiet code
                switch(server->GetState())
                {
                    case GameServer::State::RUNNING_GAME:
                        m_oMsgBuilder << "Running \t";
                        break;
                    case GameServer::State::LOBBY_FORMING:
                        m_oMsgBuilder << "Forming \t";
                        break;
                    case GameServer::State::HERO_PICK:
                        m_oMsgBuilder << "Hero pick \t";
                        break;
                    case GameServer::State::FINISHED:
                        m_oMsgBuilder << "Finished \t";
                        break;
                }
                m_oMsgBuilder << "Port " << server->GetConfig().Port << "\t";
                m_oMsgBuilder << "Players " << server->GetConfig().Players << "\t";
                m_oMsgBuilder << "Random seed " << server->GetConfig().RandomSeed;
                m_oMsgBuilder << "\n";
            }
            m_oLogSys.Info(m_oMsgBuilder.str());
            m_oMsgBuilder.str("");
        }
        else
        {
            m_oMsgBuilder << "Undefined command \'" << command << "\'";
            m_oLogSys.Warning(m_oMsgBuilder.str());
            m_oMsgBuilder.str("");
        }
    }
}
