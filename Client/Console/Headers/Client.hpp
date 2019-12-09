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
#define ERROR_SOCKET_WRITE                                      "Socket write error"
#define ERROR_SOCKET_READ                                       "Socket read error"
#define MESSAGE_SUCCESS                                         "SUCCESS"
#define VIGENERE_KEY_SERVER(serverIP, serverPort)               ((string)serverIP + to_string(serverPort))
#define VIGENERE_KEY_CLIENT(clientIP, clientMAC)                ((string)clientIP + clientMAC)
#define VIGENERE_RANDOM_PREFIX_LENGTH                           32
#define VIGENERE_RANDOM_SUFFIX_LENGTH                           32

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
    public:    class AdministratorCredentials;

    private:   Client();

    public:    SuccessState Connect(string serverIP, unsigned int serverPort);
    public:    SuccessState Disconnect();

    protected: virtual SuccessState GetCommandToSend();
    protected: virtual AdministratorCredentials GetAdministratorCredentials();
    protected: static string GetMacAddress();

    private:   int serverSocket;
    private:   string serverIP;
    private:   unsigned int serverPort;
    private:   string clientIP;
    private:   string clientMAC;
    private:   bool isConnected;

    public:    const string serverIP_Get();
    public:    const unsigned int serverPort_Get();
    public:    const string clientIP_Get();
    public:    const string clientMAC_Get();
    public:    const bool isConnected_Get();

    private:   static Client * singletonInstance;
    public:    static const Client * GetSingletonInstance();  

    public:
    class AdministratorCredentials
    {
        public: AdministratorCredentials(string adminName, string adminPassword);

        public: string adminName;
        public: string adminPassword;
    }; 
};