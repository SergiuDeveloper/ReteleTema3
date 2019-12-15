#include "../Headers/Server.hpp"

SuccessState Server::Start(unsigned int serverPort)
{
    if (this->serverRunning)
        return SuccessState(false, ERROR_SERVER_ALREADY_RUNNING);

    this->serverPort = serverPort;
    this->serverPath = INVALID_SERVER_PATH;
    this->isLocalServer = false;

    bool operationSuccess;
    
    this->serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    operationSuccess = (serverSocket != -1);
    if (!operationSuccess)
        return SuccessState(false, ERROR_SERVER_SOCKET_INITIALIZATION);

    struct sockaddr_in serverSocketAddr;
    serverSocketAddr.sin_family = AF_INET;
    serverSocketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverSocketAddr.sin_port = htons(this->serverPort);

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

SuccessState Server::Start(string serverPath)
{
    if (this->serverRunning)
        return SuccessState(false, ERROR_SERVER_ALREADY_RUNNING);

    this->serverPort = INVALID_SERVER_PORT;
    this->serverPath = serverPath;
    this->isLocalServer = true;

    bool operationSuccess;

    this->serverSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    operationSuccess = (serverSocket != -1);
    if (!operationSuccess)
        return SuccessState(false, ERROR_SERVER_SOCKET_INITIALIZATION);

    struct sockaddr_un serverSocketAddr;
    serverSocketAddr.sun_family = AF_UNIX;
    strcpy(serverSocketAddr.sun_path, this->serverPath.c_str());

    unlink(this->serverPath.c_str());
    operationSuccess = (bind(this->serverSocket, (struct sockaddr *)&serverSocketAddr, SUN_LEN(&serverSocketAddr)) != -1);
    if (!operationSuccess)
        return SuccessState(false, ERROR_SERVER_SOCKET_BINDING_PATH(this->serverPath));

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

    return SuccessState(true, SUCCESS_SERVER_STARTED_PATH(this->serverPath));
}

SuccessState Server::Stop()
{
    if (!this->serverRunning)
        return SuccessState(false, ERROR_SERVER_NOT_RUNNING);

    close(this->serverSocket);
    
    while (!pthread_mutex_trylock(&this->clientSocketsMutex));
    for (auto & clientSocket : this->clientSockets)
        close(clientSocket.clientSocketDescriptor);
    this->clientSockets.clear();
    pthread_mutex_unlock(&this->clientSocketsMutex);

    pthread_mutex_destroy(&this->clientSocketsMutex);

    if (this->isLocalServer)
        unlink(this->serverPath.c_str());

    this->serverRunning = false;

    return SuccessState(true, SUCCESS_SERVER_STOPPED);
}

void * Server::ClientsAcceptanceThreadFunction(void * threadParameters)
{
    ClientSocket clientSocket;
    struct sockaddr_in clientSocketAddr;
    socklen_t clientSocketAddrLen;

    Server::ClientConnectedThreadParameters clientConnectedThreadParameters;
    pthread_t clientHandlingThread;
    
    bool operationSuccess;
    while (this->serverRunning)
    {
        clientSocketAddrLen = sizeof(sockaddr_in);
        operationSuccess = false;

        while (this->serverRunning && !operationSuccess)
        {
            clientSocket.clientSocketDescriptor = accept(this->serverSocket, this->isLocalServer ? nullptr : (struct sockaddr *)&clientSocketAddr, this->isLocalServer ? nullptr : &clientSocketAddrLen);
            operationSuccess = (clientSocket.clientSocketDescriptor > 0);
        }

        if (this->serverRunning && operationSuccess)
        {
            clientSocket.clientSocketAddr = clientSocketAddr;

            clientConnectedThreadParameters = Server::ClientConnectedThreadParameters(this, clientSocket, clientSocketAddr);

            pthread_create(&clientHandlingThread, nullptr, (FunctionPointer)&Server::ClientHandlingThreadFunctionHelper, &clientConnectedThreadParameters);
            pthread_detach(clientHandlingThread);
        }
    }

    return nullptr;
}

void * Server::ClientHandlingThreadFunctionHelper(void * threadParameters)
{
    Server::ClientConnectedThreadParameters * clientConnectedThreadParameters = (Server::ClientConnectedThreadParameters *)threadParameters;
    clientConnectedThreadParameters->serverInstance->ClientHandlingThreadFunction(* clientConnectedThreadParameters);
}

void * Server::ClientHandlingThreadFunction(Server::ClientConnectedThreadParameters clientConnectedThreadParameters)
{
    ClientSocket clientSocket = clientConnectedThreadParameters.clientSocket;
    if (!this->isLocalServer)
    {
        struct pollfd pollDescriptor;
        pollDescriptor.fd = clientSocket.clientSocketDescriptor;
        pollDescriptor.events = POLLIN;
        int pollReturnValue = poll(&pollDescriptor, 1, DEFAULT_RECV_TIMEOUT);
        
        bool operationSuccess;
        operationSuccess = (pollReturnValue > 0);
        if (!operationSuccess)
        {
            close(clientSocket.clientSocketDescriptor);
            return nullptr;
        }
        size_t macAddressSize;
        operationSuccess = (read(clientSocket.clientSocketDescriptor, &macAddressSize, sizeof(size_t)) > 0);
        if (!operationSuccess)
        {
            close(clientSocket.clientSocketDescriptor);
            return nullptr;
        }

        pollReturnValue = poll(&pollDescriptor, 1, DEFAULT_RECV_TIMEOUT);
        operationSuccess = (pollReturnValue > 0);
        if (!operationSuccess)
        {
            close(clientSocket.clientSocketDescriptor);
            return nullptr;
        }
        char * clientMAC = new char[macAddressSize + 1];
        operationSuccess = (read(clientSocket.clientSocketDescriptor, clientMAC, macAddressSize) > 0);
        if (!operationSuccess)
        {
            close(clientSocket.clientSocketDescriptor);
            return nullptr;
        }
        clientMAC[macAddressSize] = '\0';

        clientSocket.clientIP = this->isLocalServer ? nullptr : inet_ntoa(clientConnectedThreadParameters.clientSocketAddr.sin_addr);
        clientSocket.clientMAC = clientMAC;
    }

    while (!pthread_mutex_trylock(&this->clientSocketsMutex));
    this->clientSockets.push_back(clientSocket);
    pthread_mutex_unlock(&this->clientSocketsMutex);

    this->ClientConnected_EventCallback(clientSocket);
}

const unsigned int Server::serverPort_Get()
{
    if (this->isLocalServer)
        return INVALID_SERVER_PORT;

    return this->serverPort;
}

const string Server::serverPath_Get()
{
    if (!this->isLocalServer)
        return INVALID_SERVER_PATH;

    return this->serverPath;
}

const bool Server::serverRunning_Get()
{
    return this->serverRunning;
}

Server::ClientConnectedThreadParameters::ClientConnectedThreadParameters()
{
}

Server::ClientConnectedThreadParameters::ClientConnectedThreadParameters(Server * serverInstance, ClientSocket clientSocket, struct sockaddr_in clientSocketAddr) : serverInstance(serverInstance), clientSocket(clientSocket),
    clientSocketAddr(clientSocketAddr)
{
}