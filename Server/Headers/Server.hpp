#pragma once

#define SUCCESS_SERVER_STARTED(serverPort)              ((string)"Server successfully started on port " + to_string(serverPort))
#define SUCCESS_SERVER_STARTED_PATH(serverPath)         ((string)"Server successfully started on path " + serverPath)
#define SUCCESS_SERVER_STOPPED                          "Server successfully stopped"
#define ERROR_SERVER_ALREADY_RUNNING                    "An instance of the server is already running"
#define ERROR_SERVER_NOT_RUNNING                        "There are no running instances of the server"
#define ERROR_SERVER_SOCKET_INITIALIZATION              "Could not initialize server socket"
#define ERROR_SERVER_SOCKET_BINDING(serverPort)         ((string)"Could not bind server socket to port " + to_string(serverPort))
#define ERROR_SERVER_SOCKET_BINDING_PATH(serverPath)    ((string)"Could not bind server socket to path " + serverPath)
#define ERROR_SERVER_SOCKET_LISTENING                   "Failed to listen to upcoming connections"
#define DEFAULT_RECV_TIMEOUT                            5000
#define INVALID_SERVER_PORT                             -1
#define INVALID_SERVER_PATH                             ""

#include <sys/socket.h>
#include <sys/unistd.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <string>
#include <vector>
#include <functional>
#include "ClientSocket.hpp"
#include "SuccessState.hpp"

typedef void * (* FunctionPointer)(void *);

using namespace std;

class Server
{
    private:
    class ClientConnectedThreadParameters
    {
        public: ClientConnectedThreadParameters();
        public: ClientConnectedThreadParameters(Server * serverInstance, ClientSocket clientSocket, struct sockaddr_in clientSocketAddr);

        public: Server * serverInstance;
        public: ClientSocket clientSocket;
        public: struct sockaddr_in clientSocketAddr;
    };

    public:    SuccessState Start(unsigned int serverPort);
    public:    SuccessState Start(string serverPath);
    public:    SuccessState Stop();

    protected: virtual void ClientConnected_EventCallback(ClientSocket clientSocket) = 0;

    private:   unsigned int serverPort;
    private:   string serverPath;
    private:   bool isLocalServer;
    private:   bool serverRunning;
    protected: int serverSocket;
    protected: vector<ClientSocket> clientSockets;
    protected: pthread_mutex_t clientSocketsMutex;

    protected: pthread_t clientsAcceptanceThread;
    private:   void * ClientsAcceptanceThreadFunction(void * threadParameters);
    private:   static void * ClientHandlingThreadFunctionHelper(void * threadParameters);
    private:   void * ClientHandlingThreadFunction(Server::ClientConnectedThreadParameters clientConnectedThreadParameters);

    public:    const unsigned int serverPort_Get();
    public:    const string serverPath_Get();
    public:    const bool serverRunning_Get();
};