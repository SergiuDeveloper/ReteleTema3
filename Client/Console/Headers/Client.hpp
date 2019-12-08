#pragma once

#define PROMPT_ENTER_ADMINISTRATOR_USERNAME                     "Please enter your network administrator username: "
#define PROMPT_ENTER_ADMINISTRATOR_PASSWORD                     "Please enter your network administrator password: "
#define SUCCESS_CONNECTION_ESTABLISHED(serverIP, serverPort)    ((string)"Successfully connected to " + serverIP + ":" + to_string(serverPort))
#define SUCCESS_CONNECTION_CLOSED(serverIP, serverPort)         ((string)"Failed to connect to " + serverIP + ":" + to_string(serverPort))
#define ERROR_CONNECTION_INTRERUPTED                            "Connection with the server intrerupted"
#define ERROR_CONNECTION_ALREADY_ESTABLISHED                    "There is already one instance of the client connected to the server"
#define ERROR_NO_CONNECTION_ESTABLISHED                         "There are no instances of the client connected to the server"
#define ERROR_COULD_NOT_INITIALIZE_SOCKET                       "Could not create a socket descriptor for the client"
#define ERROR_CONNECTION_FAILED(serverIP, serverPort)           ((string)"Could not connect to " + serverIP + ":" + to_string(serverPort))

#include <sys/socket.h>
#include <sys/unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <string.h>
#include <iostream>
#include <string>
#include "Encryption.hpp"
#include "SuccessState.hpp"

using namespace std;

class Client
{
    public:
    class AdministratorCredentials
    {
        public: AdministratorCredentials(string adminName, string adminPassword);

        public: string adminName;
        public: string adminPassword;
    };

    private:   Client();

    public:    SuccessState Connect(string serverIP, unsigned int serverPort);
    public:    SuccessState Disconnect();

    protected: virtual AdministratorCredentials GetAdministratorCredentials();
    protected: static string GetMacAddress();

    private:   int serverSocket;
    private:   string serverIP;
    private:   unsigned int serverPort;
    private:   bool isConnected;

    public:    const string serverIP_Get();
    public:    const unsigned int serverPort_Get();
    public:    const bool isConnected_Get();

    private:   static Client * singletonInstance;
    public:    static const Client * GetSingletonInstance();   
};