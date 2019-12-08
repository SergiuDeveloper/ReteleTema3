#pragma once

#define SUCCESS_CONNECTION_ESTABLISHED                  "Connection to the server successfully established"
#define SUCCESS_CONNECTION_CLOSED                       "Connection to the server successfully closed"
#define ERROR_CONNECTION_ALREADY_ESTABLISHED            "There is already one instance of the client connected to the server"
#define ERROR_NO_CONNECTION_ESTABLISHED                 "There are no instances of the client connected to the server"
#define ERROR_COULD_NOT_INITIALIZE_SOCKET               "Could not create a socket descriptor for the client"
#define ERROR_CONNECTION_FAILED(serverIP, serverPort)   ((string)"Could not connect to " + serverIP + ":" + to_string(serverPort));

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/unistd.h>
#include <string>
#include "Encryption.hpp"
#include "SuccessState.hpp"

using namespace std;

class Client
{
    public:    SuccessState Connect(string serverIP, unsigned int serverPort);
    public:    SuccessState Disconnect();

    protected: int serverSocket;
    private:   string serverIP;
    private:   unsigned int serverPort;
    protected: bool isConnected;

    public:    const string serverIP_Get();
    public:    const unsigned int serverPort_Get();
    public:    const bool isConnected_Get();

    private:   static Client * singletonInstance;
    public:    static const Client * GetSingletonInstance();   
};