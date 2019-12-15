#include <pthread.h>

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
    public:  static bool Start();
    public:  static bool Stop();

    public:  static bool AddWhitelistedClient(int whitelistedClient);

    private: static void GatherDisplayInfo();

    private: static int serverSocket;
    private: static unsigned int serverPort;
    private: static bool isRunning;
    private: static vector<int> whitelistedClientsVector;
    private: static pthread_mutex_t whitelistedIPsVectorMutex;
    private: static Display * serverDisplay;
    private: static XColor *** colorArray;

    private: static void * StreamDisplayThreadFunc(void * threadArguments);

    public:  static const unsigned int serverPort_Get();
    public:  static const bool isRunning_Get();
};