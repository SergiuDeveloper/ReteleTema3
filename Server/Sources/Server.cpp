#include "../Headers/Server.hpp"

unsigned int Server::serverPort;
bool Server::serverRunning = false;
int Server::serverSocket;
vector<ClientSocket> Server::clientSockets;
pthread_mutex_t Server::clientSocketsMutex;
pthread_t Server::clientsAcceptanceThread;
function<void(ClientSocket)> Server::ClientConnected_EventCallback;

SuccessState Server::Start(unsigned int serverPort, function<void(ClientSocket)> ClientConnected_EventCallback)
{
    if (Server::serverRunning)
        return SuccessState(false, ERROR_SERVER_ALREADY_RUNNING);

    Server::serverPort = serverPort;
    Server::ClientConnected_EventCallback = ClientConnected_EventCallback;

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
    Server::clientSockets.clear();
    int serverSocketFlags = fcntl(Server::serverSocket, F_GETFL, 0) | O_NONBLOCK;
    fcntl(Server::serverSocket, F_SETFL, serverSocketFlags);

    pthread_mutex_init(&Server::clientSocketsMutex, nullptr);
    pthread_create(&Server::clientsAcceptanceThread, nullptr, Server::ClientsAcceptanceThreadFunction, nullptr);
    pthread_detach(Server::clientsAcceptanceThread);

    return SuccessState(true, SUCCESS_SERVER_STARTED(Server::serverPort));
}

SuccessState Server::Stop()
{
    if (!Server::serverRunning)
        return SuccessState(false, ERROR_SERVER_NOT_RUNNING);

    close(serverSocket);
    Server::serverRunning = false;

    while (!pthread_mutex_trylock(&Server::clientSocketsMutex));
    for (auto & clientSocket : Server::clientSockets)
        close(clientSocket.clientSocketDescriptor);
    Server::clientSockets.clear();
    pthread_mutex_unlock(&Server::clientSocketsMutex);

    pthread_mutex_destroy(&Server::clientSocketsMutex);

    return SuccessState(true, SUCCESS_SERVER_STOPPED);
}

void * Server::ClientsAcceptanceThreadFunction(void * threadParameters)
{
    ClientSocket clientSocket;
    struct sockaddr_in clientSockAddr;
    socklen_t clientSocketAddrLen;
    
    bool operationSuccess;
    while (Server::serverRunning)
    {
        clientSocketAddrLen = sizeof(sockaddr_in);
        operationSuccess = false;

        while (Server::serverRunning && !operationSuccess)
        {
            clientSocket.clientSocketDescriptor = accept(Server::serverSocket, (struct sockaddr *)&clientSockAddr, &clientSocketAddrLen);
            operationSuccess = (clientSocket.clientSocketDescriptor > 0);
        }

        if (Server::serverRunning && operationSuccess)
        {
            clientSocket.clientIP = inet_ntoa(clientSockAddr.sin_addr);

            while (!pthread_mutex_trylock(&Server::clientSocketsMutex));
            Server::clientSockets.push_back(clientSocket);
            pthread_mutex_unlock(&Server::clientSocketsMutex);

            Server::ClientConnected_EventCallback(clientSocket);
        }
    }

    return nullptr;
}

unsigned int Server::serverPort_Get()
{
    return Server::serverPort;
}

bool Server::serverRunning_Get()
{
    return Server::serverRunning;
}