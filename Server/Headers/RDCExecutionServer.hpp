#pragma once

#define INVALID_BUFFER ""

#include <vector>
#include <string>
#include <X11/Xlib.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>

using namespace std;

class RDCExecutionServer
{
    public:  static bool IsGraphicsCompatible();

    public:  static bool Start();
    public:  static bool Stop();
    public:  static void AddWhitelistedIP(string whitelistedIP);

    private: static int serverSocket;
    private: static unsigned int serverPort;
    private: static bool isRunning;
    private: static vector<int> clientSockets;
    private: static pthread_mutex_t clientSocketsMutex;
    private: static vector<string> whitelistedIPs;
    private: static pthread_mutex_t whitelistedIPsMutex;

    private: static void * ClientAcceptanceThreadFunc(void * threadArguments);

    public:  static unsigned int serverPort_Get();
    public:  static bool isRunning_Get();
};