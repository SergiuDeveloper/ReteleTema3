#pragma once

#include "Server.hpp"
#include "SuccessState.hpp"

class SpecializedServer : public Server
{
    public: static SuccessState Start(unsigned int serverPort);
    
    private: static void ClientConnected_EventCallback();
};