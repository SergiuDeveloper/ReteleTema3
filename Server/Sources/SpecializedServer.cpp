#include "../Headers/SpecializedServer.hpp"

SuccessState SpecializedServer::Start(unsigned int serverPort)
{
    return Server::Start(serverPort, SpecializedServer::ClientConnected_EventCallback);
}

void SpecializedServer::ClientConnected_EventCallback()
{
}