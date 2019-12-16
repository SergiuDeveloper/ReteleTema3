#pragma once

#define SOCKET_BUFFER_LENGTH 4096

#include <vector>
#include <string>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/unistd.h>
#include <pthread.h>
#include "SuccessState.hpp"

using namespace std;

class RDCStreamingServer
{
    public:  static bool IsGraphicsCompatible();
    public:  static bool Start();
    public:  static bool Stop();

    public:  static bool AddWhitelistedClient(struct sockaddr_in clientSocketAddr);

    private: static void SerializeColorArray(int screenHeight, int screenWidth);

    private: static int serverSocket;
    private: static unsigned int serverPort;
    private: static bool isRunning;
    private: static vector<string> whitelistedIPsVector;
    private: static pthread_mutex_t whitelistedIPsVectorMutex;
    private: static Display * serverDisplay;
    private: static unsigned long ** colorArray;
    private: static vector<string> colorArraySerialized;

    private: static void * GatherDisplayInfoThreadFunc(void * threadArguments);
    private: static void * ReceiveConnectionsThreadFunc(void * threadArguments);
    private: static void * StreamDisplayThreadFunc(void * threadArguments);

    public:  static const unsigned int serverPort_Get();
    public:  static const bool isRunning_Get();
};