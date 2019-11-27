#include "../Headers/ClientSocket.hpp"

ClientSocket::ClientSocket()
{
}

ClientSocket::ClientSocket(int clientSocketDescriptor, struct sockaddr_in clientSocketAddr) : clientSocketDescriptor(clientSocketDescriptor), clientSocketAddr(clientSocketAddr)
{
}