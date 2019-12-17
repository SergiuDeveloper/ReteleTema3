#include "../Headers/RDCExecutionServer.hpp"

int RDCExecutionServer::serverSocket;
unsigned int RDCExecutionServer::serverPort;
bool RDCExecutionServer::isRunning = false;
vector<int> RDCExecutionServer::clientSockets;
pthread_mutex_t RDCExecutionServer::clientSocketsMutex;
vector<string> RDCExecutionServer::whitelistedIPs;
pthread_mutex_t RDCExecutionServer::whitelistedIPsMutex;
char RDCExecutionServer::keyboardState[KEYBOARD_STATE_COUNT];
pthread_mutex_t RDCExecutionServer::keyboardStateMutex;

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
    pthread_mutex_init(&RDCExecutionServer::keyboardStateMutex, nullptr);

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

    RDCExecutionServer::isRunning = true;

    pthread_t keyboardStateThread;
    pthread_create(&keyboardStateThread, nullptr, RDCExecutionServer::KeyboardStateThreadFunc, nullptr);
    pthread_detach(keyboardStateThread);

    pthread_t clientAcceptanceThread;
    pthread_create(&clientAcceptanceThread, nullptr, RDCExecutionServer::ClientAcceptanceThreadFunc, nullptr);
    pthread_detach(clientAcceptanceThread);

    socklen_t serverSocketAddrLength = sizeof(serverSocketAddr);
    getsockname(RDCExecutionServer::serverSocket, (struct sockaddr *)&serverSocketAddr, &serverSocketAddrLength);

    RDCExecutionServer::serverPort = ntohs(serverSocketAddr.sin_port);

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

    pthread_mutex_destroy(&RDCExecutionServer::keyboardStateMutex);
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

                pthread_t clientExecutionThread;
                pthread_create(&clientExecutionThread, nullptr, RDCExecutionServer::ClientExecutionThreadFunc, &clientSocket);
                pthread_detach(clientExecutionThread);

                continue;
            }
    }

    return nullptr;
}

void * RDCExecutionServer::ClientExecutionThreadFunc(void * threadArguments)
{
    int clientSocket = * (int *)threadArguments;

    bool clientConnected = true;
    size_t keyboardStateLength = KEYBOARD_STATE_COUNT;
    bool clientResponse;

    write(RDCExecutionServer::serverSocket, &keyboardStateLength, sizeof(size_t));
    
    while (RDCExecutionServer::isRunning && clientConnected)
    {
        while (!pthread_mutex_trylock(&RDCExecutionServer::keyboardStateMutex));
        write(clientSocket, RDCExecutionServer::keyboardState, keyboardStateLength);
        pthread_mutex_unlock(&RDCExecutionServer::keyboardStateMutex);

        clientConnected = (read(clientSocket, &clientResponse, sizeof(clientResponse)) != 0);
    }

    close(clientSocket);

    while (!pthread_mutex_trylock(&RDCExecutionServer::clientSocketsMutex));
    for (size_t clientSocketsIterator = 0; clientSocketsIterator < RDCExecutionServer::clientSockets.size(); ++clientSocketsIterator)
        if (RDCExecutionServer::clientSockets[clientSocketsIterator] == clientSocket)
        {
            RDCExecutionServer::clientSockets[clientSocketsIterator] = RDCExecutionServer::clientSockets[RDCExecutionServer::clientSockets.size() - 1];
            RDCExecutionServer::clientSockets.pop_back();

            pthread_mutex_unlock(&RDCExecutionServer::clientSocketsMutex);
            return nullptr;
        }

    pthread_mutex_unlock(&RDCExecutionServer::clientSocketsMutex);
    return nullptr;
}

void * RDCExecutionServer::KeyboardStateThreadFunc(void * threadArguments)
{
    Display * defaultDisplay = XOpenDisplay(nullptr);
    if (defaultDisplay == nullptr)
        return nullptr;

    char keyboardStateCopy[KEYBOARD_STATE_COUNT];
    while (RDCExecutionServer::isRunning)
        if (RDCExecutionServer::clientSockets.size() > 0)
        {
            XQueryKeymap(defaultDisplay, keyboardStateCopy);

            while (!pthread_mutex_trylock(&RDCExecutionServer::keyboardStateMutex));
            for (size_t keyboardStateCopyIterator = 0; keyboardStateCopyIterator < KEYBOARD_STATE_COUNT; ++keyboardStateCopyIterator)
                RDCExecutionServer::keyboardState[keyboardStateCopyIterator] = keyboardStateCopy[keyboardStateCopyIterator];
            pthread_mutex_unlock(&RDCExecutionServer::keyboardStateMutex);
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