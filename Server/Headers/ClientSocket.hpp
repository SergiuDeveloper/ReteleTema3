#pragma once

#include <netinet/in.h>
#include <string>

using namespace std;

class ClientSocket
{
    public: ClientSocket();
    public: ClientSocket(int clientSocketDescriptor, string clientIP);

    public: int clientSocketDescriptor;
    public: string clientIP;
};