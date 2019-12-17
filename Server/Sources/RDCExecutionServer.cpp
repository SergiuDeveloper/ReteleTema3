#include "../Headers/RDCExecutionServer.hpp"

int RDCExecutionServer::serverSocket;
unsigned int RDCExecutionServer::serverPort;
bool RDCExecutionServer::isRunning = false;
vector<int> RDCExecutionServer::clientSockets;
pthread_mutex_t RDCExecutionServer::clientSocketsMutex;
vector<string> RDCExecutionServer::whitelistedIPs;
pthread_mutex_t RDCExecutionServer::whitelistedIPsMutex;

bool RDCExecutionServer::IsGraphicsCompatible()
{
    Display * xDisplay = XOpenDisplay(nullptr);
    bool isGraphicsCompatible = (xDisplay != nullptr);

    return isGraphicsCompatible;
}

bool RDCExecutionServer::Start()
{
    if (RDCExecutionServer::isRunning)
        return false;

    if (!RDCExecutionServer::IsGraphicsCompatible())
        return false;

    pthread_mutex_init(&RDCExecutionServer::whitelistedIPsMutex, nullptr);
    pthread_mutex_init(&RDCExecutionServer::clientSocketsMutex, nullptr);
    RDCExecutionServer::whitelistedIPs.clear();
    RDCExecutionServer::clientSockets.clear();

    bool operationSuccess = ((RDCExecutionServer::serverSocket = socket(AF_INET, SOCK_STREAM, 0)) != -1);
    if (!operationSuccess)
        return false;

    struct sockaddr_in serverSocketAddr;
    serverSocketAddr.sin_family = AF_INET;
    serverSocketAddr.sin_addr.s_addr = INADDR_ANY;
    serverSocketAddr.sin_port = htons(0);

    operationSuccess = (bind(RDCExecutionServer::serverSocket, (struct sockaddr *)&serverSocketAddr, sizeof(serverSocketAddr)) != -1);
    if (!operationSuccess)
    {
        close(RDCExecutionServer::serverSocket);
        return false;
    }

    operationSuccess = (listen(RDCExecutionServer::serverSocket, SOMAXCONN) != -1);
    if (!operationSuccess)
    {
        close(RDCExecutionServer::serverSocket);
        return false;
    }

    int serverSocketFlags = fcntl(RDCExecutionServer::serverSocket, F_GETFL, 0) | O_NONBLOCK;
    fcntl(RDCExecutionServer::serverSocket, F_SETFL, serverSocketFlags);

    pthread_t clientAcceptanceThread;
    pthread_create(&clientAcceptanceThread, nullptr, RDCExecutionServer::ClientAcceptanceThreadFunc, nullptr);
    pthread_detach(clientAcceptanceThread);

    RDCExecutionServer::isRunning = true;

    socklen_t serverSocketAddrLength = sizeof(serverSocketAddr);
    getsockname(RDCExecutionServer::serverSocket, (struct sockaddr *)&serverSocketAddr, &serverSocketAddrLength);

    return true;
}

bool RDCExecutionServer::Stop()
{
    if (!RDCExecutionServer::isRunning)
        return false;

    close(RDCExecutionServer::serverSocket);

    while (!pthread_mutex_trylock(&RDCExecutionServer::clientSocketsMutex));
    for (auto & clientSocket : RDCExecutionServer::clientSockets)
        close(clientSocket);
    pthread_mutex_unlock(&RDCExecutionServer::clientSocketsMutex);

    pthread_mutex_destroy(&RDCExecutionServer::whitelistedIPsMutex);
    pthread_mutex_destroy(&RDCExecutionServer::clientSocketsMutex);

    RDCExecutionServer::isRunning = false;

    return true;
}

bool RDCExecutionServer::AddWhitelistedIP(string whitelistedIP)
{
    if (!RDCExecutionServer::isRunning)
        return false;

    while (!pthread_mutex_trylock(&RDCExecutionServer::whitelistedIPsMutex));
    RDCExecutionServer::whitelistedIPs.push_back(whitelistedIP);
    pthread_mutex_unlock(&RDCExecutionServer::whitelistedIPsMutex);

    return true;
}

bool RDCExecutionServer::RemoveWhitelistedIP(string whitelistedIP)
{
    if (!RDCExecutionServer::isRunning)
        return false;

    while (!pthread_mutex_trylock(&RDCExecutionServer::whitelistedIPsMutex));
    for (size_t whitelistedIPsIterator = 0; whitelistedIPsIterator < RDCExecutionServer::whitelistedIPs.size(); ++whitelistedIPsIterator)
        if (RDCExecutionServer::whitelistedIPs[whitelistedIPsIterator] == whitelistedIP)
        {
            RDCExecutionServer::whitelistedIPs[whitelistedIPsIterator] = RDCExecutionServer::whitelistedIPs[RDCExecutionServer::whitelistedIPs.size() - 1];
            RDCExecutionServer::whitelistedIPs.pop_back();

            pthread_mutex_unlock(&RDCExecutionServer::whitelistedIPsMutex);
            return true;
        }
    pthread_mutex_unlock(&RDCExecutionServer::whitelistedIPsMutex);

    return false;
}

void * RDCExecutionServer::ClientAcceptanceThreadFunc(void * threadArguments)
{
    struct sockaddr_in clientSocketAddr;
    socklen_t clientSocketAddrLength;

    int clientSocket;
    bool operationSuccess;
    string clientIP;
    while (RDCExecutionServer::isRunning)
    {
        clientSocket = (accept(RDCExecutionServer::serverSocket, (struct sockaddr *)&clientSocketAddr, &clientSocketAddrLength));

        operationSuccess = (clientSocket > 0);
        if (!operationSuccess)
            continue;

        clientIP = inet_ntoa(clientSocketAddr.sin_addr);

        for (size_t whitelistedIPsIterator = 0; whitelistedIPsIterator < RDCExecutionServer::whitelistedIPs.size(); ++whitelistedIPsIterator)
            if (RDCExecutionServer::whitelistedIPs[whitelistedIPsIterator] == clientIP)
            {
                while (!pthread_mutex_trylock(&RDCExecutionServer::whitelistedIPsMutex));
                RDCExecutionServer::whitelistedIPs[whitelistedIPsIterator] = RDCExecutionServer::whitelistedIPs[RDCExecutionServer::whitelistedIPs.size() - 1];
                RDCExecutionServer::whitelistedIPs.pop_back();
                pthread_mutex_unlock(&RDCExecutionServer::whitelistedIPsMutex);

                while (!pthread_mutex_trylock(&RDCExecutionServer::clientSocketsMutex));
                RDCExecutionServer::clientSockets.push_back(clientSocket);
                pthread_mutex_unlock(&RDCExecutionServer::clientSocketsMutex);

                continue;
            }
    }

    return nullptr;
}

unsigned int RDCExecutionServer::serverPort_Get()
{
    return RDCExecutionServer::serverPort;
}

bool RDCExecutionServer::isRunning_Get()
{
    return RDCExecutionServer::isRunning;
}