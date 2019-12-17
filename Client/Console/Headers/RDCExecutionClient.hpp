#pragma once

#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

using namespace std;

class RDCExecutionClient
{
    public:  static bool IsGraphicsCompatible();

    public:  static bool Start(string serverIP, unsigned int serverPort, Window graphicsWindow);
    public:  static bool Stop();

    private: static string serverIP;
    private: static unsigned int serverPort;
    private: static int serverSocket;
    private: static bool isRunning;
    private: static char * previousKeyboardState;
    private: static char * keyboardState;
    private: static size_t keyboardStateLength;
    private: static pthread_mutex_t keyboardStateMutex;
    private: static Window graphicsWindow;

    private: static void * ReceiveActionsThreadFunc(void * threadArguments);
    private: static void * ExecuteActionsThreadFunc(void * threadArguments);

    public:  static string serverIP_Get();
    public:  static unsigned int serverPort_Get();
    public:  static bool isRunning_Get();
};