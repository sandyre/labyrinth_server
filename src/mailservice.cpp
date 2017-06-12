//
//  mailservice.cpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 13.04.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#include "mailservice.hpp"

MailService::MailService()
{
    
}

MailService::State
MailService::GetState() const
{
    return m_eState;
}

void
MailService::init()
{
    try
    {
        m_poSMTPSession = std::make_unique<Poco::Net::SMTPClientSession>("smtp.timeweb.ru",
                                                                        25);
        m_poSMTPSession->login(Poco::Net::SMTPClientSession::LoginMethod::AUTH_LOGIN,
                              "noreply-labyrinth@hate-red.com",
                              "VdAysb4A");
        
        m_eState = State::ESTABLISHED;
    }
    catch(std::exception& e)
    {
        m_eState = State::FAILED;
    }
}

void
MailService::PushMessage(const Poco::Net::MailMessage& msg)
{
    std::lock_guard<std::mutex> locker(m_oQueueMutex);
    
        // FIXME: COSTIL PIZDETS! SHOULD BE FIXED.
    
    m_aMessageQueue.emplace();
    auto& p_msg = m_aMessageQueue.back();
    p_msg.setContent(msg.getContent());
    p_msg.setRecipients(msg.recipients());
    p_msg.setSubject(msg.getSubject());
}

void
MailService::run(Poco::Timer& timer)
{
    std::lock_guard<std::mutex> locker(m_oQueueMutex);
    
    while(m_aMessageQueue.size())
    {
        m_poSMTPSession->sendMessage(m_aMessageQueue.front());
        m_aMessageQueue.pop();
    }
}
