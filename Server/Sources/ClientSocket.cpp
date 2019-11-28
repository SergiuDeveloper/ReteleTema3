#include "../Headers/ClientSocket.hpp"

ClientSocket::ClientSocket()
{
}

ClientSocket::ClientSocket(int clientSocketDescriptor, string clientIP) : clientSocketDescriptor(clientSocketDescriptor), clientIP(clientIP)
{
}