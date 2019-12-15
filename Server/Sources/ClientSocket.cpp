#include "../Headers/ClientSocket.hpp"

ClientSocket::ClientSocket()
{
}

ClientSocket::ClientSocket(int clientSocketDescriptor, struct sockaddr_in clientSocketAddr, string clientIP, string clientMAC) : clientSocketDescriptor(clientSocketDescriptor), clientSocketAddr(clientSocketAddr), clientIP(clientIP),
    clientMAC(clientMAC)
{
}