#pragma once

#define LOCAL_SERVER_PORT 2000
#define LOCAL_IP          "127.0.0.1"

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