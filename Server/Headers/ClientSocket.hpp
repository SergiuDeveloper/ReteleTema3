#pragma once

#include <netinet/in.h>

class ClientSocket
{
    public: ClientSocket();
    public: ClientSocket(int clientSocketDescriptor, struct sockaddr_in clientSocketAddr);

    public: int clientSocketDescriptor;
    public: struct sockaddr_in clientSocketAddr;
};