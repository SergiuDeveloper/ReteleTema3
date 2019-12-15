#pragma once

#include <netinet/in.h>
#include <string>

using namespace std;

class ClientSocket
{
    public: ClientSocket();
    public: ClientSocket(int clientSocketDescriptor, struct sockaddr_in clientSocektAddr, string clientIP, string clientMAC);

    public: int clientSocketDescriptor;
    public: struct sockaddr_in clientSocketAddr;
    public: string clientIP;
    public: string clientMAC;
};