//
//  mailservice.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 13.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef mailservice_hpp
#define mailservice_hpp

#include <queue>
#include <mutex>
#include <memory>
#include <Poco/Net/SMTPClientSession.h>
#include <Poco/Net/MailMessage.h>
#include <Poco/Runnable.h>
#include <Poco/Timer.h>

class MailService
{
public:
    enum State
    {
        ESTABLISHED,
        FAILED
    };
public:
    MailService();
    
    void    init();
    void    run(Poco::Timer& timer); // not a while cycle, should be called by another object
    
    State   GetState() const;
    void    PushMessage(const Poco::Net::MailMessage&);
private:
    MailService::State  m_eState;
    std::unique_ptr<Poco::Net::SMTPClientSession> m_poSMTPSession;
    std::mutex  m_oQueueMutex;
    std::queue<Poco::Net::MailMessage> m_aMessageQueue;
};

#endif /* mailservice_hpp */
