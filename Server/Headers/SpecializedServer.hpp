#pragma once

#define SUCCESS_CLIENT_CONNECTED(clientIP) ((string)"Client " + clientIP + " successfully connected!")

#include <iostream>
#include "Server.hpp"
#include "ClientSocket.hpp"
#include "SuccessState.hpp"

using namespace std;

class SpecializedServer : public Server
{
    public: static SuccessState Start(unsigned int serverPort);
    
    private: static void ClientConnected_EventCallback(ClientSocket clientSocket);
};