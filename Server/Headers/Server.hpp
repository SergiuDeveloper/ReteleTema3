#pragma once

#define SUCCESS_SERVER_STARTED(serverPort)      ((string)"Server successfully started on port " + to_string(serverPort))
#define SUCCESS_SERVER_STOPPED                  "Server successfully stopped"
#define ERROR_SERVER_ALREADY_RUNNING            "An instance of the server is already running"
#define ERROR_SERVER_NOT_RUNNING                "There are no running instances of the server"
#define ERROR_SERVER_SOCKET_INITIALIZATION      "Could not initialize server socket"
#define ERROR_SERVER_SOCKET_BINDING(serverPort) ((string)"Could not bind server socket to port " + to_string(serverPort))
#define ERROR_SERVER_SOCKET_LISTENING           "Failed to listen to upcoming connections"

#include <sys/socket.h>
#include <sys/unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <string>
#include <vector>
#include <functional>
#include "ClientSocket.hpp"
#include "SuccessState.hpp"

using namespace std;

class Server
{
    public:    static SuccessState Start(unsigned int serverPort, function<void(ClientSocket)> ClientConnected_EventCallback);
    public:    static SuccessState Stop();

    private:   static unsigned int serverPort;
    private:   static bool serverRunning;
    protected: static int serverSocket;
    protected: static vector<ClientSocket> clientSockets;
    protected: static pthread_mutex_t clientSocketsMutex;
    protected: static function<void(ClientSocket)> ClientConnected_EventCallback;

    protected: static pthread_t clientsAcceptanceThread;
    private:   static void * ClientsAcceptanceThreadFunction(void * threadParameters);

    public:    static unsigned int serverPort_Get();
    public:    static bool serverRunning_Get();
};