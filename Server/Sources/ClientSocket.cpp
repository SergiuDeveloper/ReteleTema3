#include "../Headers/ClientSocket.hpp"

ClientSocket::ClientSocket()
{
}

ClientSocket::ClientSocket(int clientSocketDescriptor, string clientIP, string clientMAC) : clientSocketDescriptor(clientSocketDescriptor), clientIP(clientIP), clientMAC(clientMAC)
{
}