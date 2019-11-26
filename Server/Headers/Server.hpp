#ifndef SERVER_HPP
    #define SERVER_HPP
#endif

#define SUCCESS_SERVER_STARTED(serverPort)      ((string)"Server successfully started on port " + to_string(serverPort))
#define SUCCESS_SERVER_STOPPED                  "Server successfully stopped"
#define ERROR_SERVER_ALREADY_RUNNING            "An instance of the server is already running"
#define ERROR_SERVER_NOT_RUNNING                "There are no running instances of the server"
#define ERROR_SERVER_SOCKET_INITIALIZATION      "Could not initialize server socket"
#define ERROR_SERVER_SOCKET_BINDING(serverPort) ((string)"Could not bind server socket to port " + to_string(serverPort))
#define ERROR_SERVER_SOCKET_LISTENING           "Failed to listen to upcoming connections"

#include <sys/socket.h>
#include <sys/unistd.h>
#include <netinet/in.h>
#include <string>
#include "SuccessState.hpp"

using namespace std;

class Server
{
    public:    static SuccessState Start(unsigned int serverPort);
    public:    static SuccessState Stop();

    private:   static unsigned int serverPort;
    protected: static bool serverRunning;
    protected: static int serverSocket;

    public:    static unsigned int serverPort_Get();
    public:    static bool serverRunning_Get();
};