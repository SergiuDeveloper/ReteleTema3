#pragma once

#define SOCKET_BUFFER_LENGTH 4096

#include <string>
#include <sstream>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <arpa/inet.h>
#include <X11/Xlib.h>
#include <pthread.h>

using namespace std;

class RDCStreamingClient
{
    public:  static bool IsGraphicsCompatible();

    public:  static bool Start(string serverIP, unsigned int serverPort);
    public:  static bool Stop();

    private: static bool InitializeGraphics();

    private: static string serverIP;
    private: static unsigned int serverPort;
    private: static int serverSocket;
    private: static sockaddr_in serverSocketAddr;
    private: static bool isRunning;
    private: static Window graphicsWindow;
    private: static unsigned long ** screenColors;
    private: static int windowHeight;
    private: static int windowWidth;

    private: static void * GraphicsHandlingThreadFunc(void * threadArguments);
    private: static void * GetGraphicsInformationThreadFunc(void * threadArguments);

    public:  static string serverIP_Get();
    public:  static unsigned int serverPort_Get();
    public:  static bool isRunning_Get();
};