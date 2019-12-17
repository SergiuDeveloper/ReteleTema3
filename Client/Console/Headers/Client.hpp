#pragma once

#define PROMPT_ENTER_ADMINISTRATOR_USERNAME                     "Please enter your network administrator username: "
#define PROMPT_ENTER_ADMINISTRATOR_PASSWORD                     "Please enter your network administrator password: "
#define SUCCESS_CONNECTION_ESTABLISHED(serverIP, serverPort)    ((string)"Successfully connected to " + serverIP + ":" + to_string(serverPort))
#define SUCCESS_CONNECTION_CLOSED(serverIP, serverPort)         ((string)"Failed to connect to " + serverIP + ":" + to_string(serverPort))
#define SUCCESS_CLIENT_LIFECYCLE                                "Successfully ended the client lifecycle"
#define SUCCESS_OPEN_RDC_STREAMING_CLIENT                       "Successfully opened the RDC streaming client"
#define SUCCESS_OPEN_RDC_EXECUTION_CLIENT                       "Successfully opened the RDC execution client"
#define ERROR_CONNECTION_INTRERUPTED                            "Connection with the server intrerupted"
#define ERROR_CONNECTION_ALREADY_ESTABLISHED                    "There is already one instance of the client connected to the server"
#define ERROR_NO_CONNECTION_ESTABLISHED                         "There are no instances of the client connected to the server"
#define ERROR_COULD_NOT_INITIALIZE_SOCKET                       "Could not create a socket descriptor for the client"
#define ERROR_CONNECTION_FAILED(serverIP, serverPort)           ((string)"Could not connect to " + serverIP + ":" + to_string(serverPort))
#define ERROR_SOCKET_WRITE                                      "Socket write error"
#define ERROR_SOCKET_READ                                       "Socket read error"
#define ERROR_COMMAND_EXECUTION                                 "Failed to execute the requested command on the server"
#define ERROR_NOT_GRAPHICS_COMPATIBLE                           "Your UNIX version doesn't have UI support"
#define ERROR_OPEN_RDC_STREAMING_CLIENT                         "Failed to open the RDC streaming client"
#define ERROR_OPEN_RDC_EXECUTION_CLIENT                         "Failed to open the RDC execution client"
#define COMMAND_QUIT_CLIENT                                     "QUIT"
#define COMMAND_HELP                                            "HELP"
#define COMMAND_CONNECT_RDC                                     "CONNECT RDC"
#define MESSAGE_SUCCESS                                         "SUCCESS"
#define MESSAGE_COMMANDS_USAGE                                  ((string)"Special commands :\n-" + COMMAND_CONNECT_RDC + "\n-" + COMMAND_HELP + "\n-" + COMMAND_QUIT_CLIENT)
#define VIGENERE_KEY(serverPort, clientMAC)                     (to_string(serverPort) + clientMAC)
#define VIGENERE_RANDOM_PREFIX_LENGTH                           32
#define VIGENERE_RANDOM_SUFFIX_LENGTH                           32
#define DEFAULT_EXECUTION_PATH                                  "./"
#define INITIALIZE_COLOR()                                      setupterm(nullptr, 1, nullptr)
#define COLOR_DEFAULT                                           tigetstr("sgr0")
#define COLOR_GREEN                                             tparm(tigetstr("setaf"), 2)
#define COLOR_BLUE                                              tparm(tigetstr("setaf"), 4)

#include <sys/socket.h>
#include <sys/unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <string.h>
#include <term.h>
#include <iostream>
#include <string>
#include "RDCStreamingClient.hpp"
#include "RDCExecutionClient.hpp"
#include "Encryption.hpp"
#include "SuccessState.hpp"

using namespace std;

class Client
{
    public:    class AdministratorCredentials;

    private:   Client();

    public:    SuccessState Connect(string serverIP, unsigned int serverPort);
    public:    SuccessState Disconnect();
    protected: virtual void ClientLifecycle(string currentUser, string commandExecutionLocation);

    protected: SuccessState GetCommandToSend();
    protected: virtual AdministratorCredentials GetAdministratorCredentials();

    protected: static string GetMacAddress();

    private:   int serverSocket;
    private:   string serverIP;
    private:   unsigned int serverPort;
    private:   string clientMAC;
    private:   bool isConnected;

    public:    const string serverIP_Get();
    public:    const unsigned int serverPort_Get();
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