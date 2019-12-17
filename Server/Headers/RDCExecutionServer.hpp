#pragma once

#define INVALID_BUFFER          ""
#define KEYBOARD_STATE_COUNT    32

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
    public:  static bool AddWhitelistedIP(string whitelistedIP);
    public:  static bool RemoveWhitelistedIP(string whitelistedIP);

    private: static int serverSocket;
    private: static unsigned int serverPort;
    private: static bool isRunning;
    private: static vector<int> clientSockets;
    private: static pthread_mutex_t clientSocketsMutex;
    private: static vector<string> whitelistedIPs;
    private: static pthread_mutex_t whitelistedIPsMutex;
    private: static char keyboardState[KEYBOARD_STATE_COUNT];
    private: static pthread_mutex_t keyboardStateMutex;

    private: static void * ClientAcceptanceThreadFunc(void * threadArguments);
    private: static void * ClientExecutionThreadFunc(void * threadArguments);
    private: static void * KeyboardStateThreadFunc(void * threadArguments);

    public:  static unsigned int serverPort_Get();
    public:  static bool isRunning_Get();
};