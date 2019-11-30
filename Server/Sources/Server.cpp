#include "../Headers/Server.hpp"

SuccessState Server::Start(unsigned int serverPort)
{
    if (this->serverRunning)
        return SuccessState(false, ERROR_SERVER_ALREADY_RUNNING);

    this->serverPort = serverPort;

    bool operationSuccess;

    this->serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    operationSuccess = (serverSocket != -1);
    if (!operationSuccess)
        return SuccessState(false, ERROR_SERVER_SOCKET_INITIALIZATION);

    struct sockaddr_in serverSocketAddr;
    serverSocketAddr.sin_family = AF_INET;
    serverSocketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverSocketAddr.sin_port = htons(serverPort);

    operationSuccess = (bind(this->serverSocket, (struct sockaddr *)&serverSocketAddr, sizeof(struct sockaddr)) != -1);
    if (!operationSuccess)
        return SuccessState(false, ERROR_SERVER_SOCKET_BINDING(this->serverPort));

    operationSuccess = (listen(this->serverSocket, SOMAXCONN) != -1);
    if (!operationSuccess)
        return SuccessState(false, ERROR_SERVER_SOCKET_LISTENING);

    this->serverRunning = true;
    this->clientSockets.clear();
    int serverSocketFlags = fcntl(this->serverSocket, F_GETFL, 0) | O_NONBLOCK;
    fcntl(this->serverSocket, F_SETFL, serverSocketFlags);

    pthread_mutex_init(&this->clientSocketsMutex, nullptr);
    pthread_create(&this->clientsAcceptanceThread, nullptr, (FunctionPointer)&Server::ClientsAcceptanceThreadFunction, this);
    pthread_detach(this->clientsAcceptanceThread);

    return SuccessState(true, SUCCESS_SERVER_STARTED(this->serverPort));
}

SuccessState Server::Stop()
{
    if (!this->serverRunning)
        return SuccessState(false, ERROR_SERVER_NOT_RUNNING);

    close(serverSocket);
    this->serverRunning = false;

    while (!pthread_mutex_trylock(&this->clientSocketsMutex));
    for (auto & clientSocket : this->clientSockets)
        close(clientSocket.clientSocketDescriptor);
    this->clientSockets.clear();
    pthread_mutex_unlock(&this->clientSocketsMutex);

    pthread_mutex_destroy(&this->clientSocketsMutex);

    return SuccessState(true, SUCCESS_SERVER_STOPPED);
}

void * Server::ClientsAcceptanceThreadFunction(void * threadParameters)
{
    ClientSocket clientSocket;
    struct sockaddr_in clientSockAddr;
    socklen_t clientSocketAddrLen;
    
    bool operationSuccess;
    while (this->serverRunning)
    {
        clientSocketAddrLen = sizeof(sockaddr_in);
        operationSuccess = false;

        while (this->serverRunning && !operationSuccess)
        {
            clientSocket.clientSocketDescriptor = accept(this->serverSocket, (struct sockaddr *)&clientSockAddr, &clientSocketAddrLen);
            operationSuccess = (clientSocket.clientSocketDescriptor > 0);
        }

        if (this->serverRunning && operationSuccess)
        {
            clientSocket.clientIP = inet_ntoa(clientSockAddr.sin_addr);

            while (!pthread_mutex_trylock(&this->clientSocketsMutex));
            this->clientSockets.push_back(clientSocket);
            pthread_mutex_unlock(&this->clientSocketsMutex);

            this->ClientConnected_EventCallback(clientSocket);
        }
    }

    return nullptr;
}

unsigned int Server::serverPort_Get()
{
    return this->serverPort;
}

bool Server::serverRunning_Get()
{
    return this->serverRunning;
}