#include "../Headers/Server.hpp"

unsigned int Server::serverPort;
bool Server::serverRunning = false;
int Server::serverSocket;

SuccessState Server::Start(unsigned int serverPort)
{
    if (Server::serverRunning)
        return SuccessState(false, ERROR_SERVER_ALREADY_RUNNING);

    Server::serverPort = serverPort;

    bool operationSuccess;

    Server::serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    operationSuccess = (serverSocket != -1);
    if (!operationSuccess)
        return SuccessState(false, ERROR_SERVER_SOCKET_INITIALIZATION);

    struct sockaddr_in serverSocketAddr;
    serverSocketAddr.sin_family = AF_INET;
    serverSocketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverSocketAddr.sin_port = htons(serverPort);

    operationSuccess = (bind(Server::serverSocket, (struct sockaddr *)&serverSocketAddr, sizeof(struct sockaddr)) != -1);
    if (!operationSuccess)
        return SuccessState(false, ERROR_SERVER_SOCKET_BINDING(Server::serverPort));

    operationSuccess = (listen(Server::serverSocket, SOMAXCONN) != -1);
    if (!operationSuccess)
        return SuccessState(false, ERROR_SERVER_SOCKET_LISTENING);

    Server::serverRunning = true;
    return SuccessState(true, SUCCESS_SERVER_STARTED(Server::serverPort));
}

SuccessState Server::Stop()
{
    if (!Server::serverRunning)
        return SuccessState(false, ERROR_SERVER_NOT_RUNNING);

    close(serverSocket);

    return SuccessState(true, SUCCESS_SERVER_STOPPED);
}

unsigned int Server::serverPort_Get()
{
    return Server::serverPort;
}

bool Server::serverRunning_Get()
{
    return Server::serverRunning;
}
