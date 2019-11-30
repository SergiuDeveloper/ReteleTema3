#pragma once

#include "Server.hpp"

class LocalServer : public Server
{
    using Server::Start;
    using Server::Stop;

    public:  SuccessState Start();
    public:  SuccessState Stop();

    private: void ClientConnected_EventCallback(ClientSocket clientSocket);
    
    private: static LocalServer * singletonInstance;
    private: static pthread_mutex_t singletonInstanceMutex;
    public:  static const LocalServer * GetSingletonInstance();
};