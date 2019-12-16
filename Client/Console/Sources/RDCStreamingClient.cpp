#include "../Headers/RDCStreamingClient.hpp"

string RDCStreamingClient::serverIP;
unsigned int RDCStreamingClient::serverPort;
int RDCStreamingClient::serverSocket;
struct sockaddr_in RDCStreamingClient::serverSocketAddr;
bool RDCStreamingClient::isRunning = false;
Window RDCStreamingClient::graphicsWindow;
unsigned long ** RDCStreamingClient::screenColors;
int RDCStreamingClient::windowHeight;
int RDCStreamingClient::windowWidth;

bool RDCStreamingClient::IsGraphicsCompatible()
{
    Display * xDisplay = XOpenDisplay(nullptr);
    bool isGraphicsCompatible = (xDisplay != nullptr);

    return isGraphicsCompatible;
}

bool RDCStreamingClient::Start(string serverIP, unsigned int serverPort)
{
    if (RDCStreamingClient::isRunning)
        return false;

    if (!RDCStreamingClient::IsGraphicsCompatible)
        return false;

    if ((RDCStreamingClient::serverSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        return false;

    RDCStreamingClient::serverSocketAddr.sin_family = AF_INET;
    RDCStreamingClient::serverSocketAddr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIP.c_str(), &RDCStreamingClient::serverSocketAddr.sin_addr);

    if (connect(RDCStreamingClient::serverSocket, (struct sockaddr *)&RDCStreamingClient::serverSocketAddr, sizeof(RDCStreamingClient::serverSocketAddr)) == -1)
        return false;

    if (!RDCStreamingClient::InitializeGraphics())
    {
        close(RDCStreamingClient::serverSocket);
        return false;
    }

    pthread_t getGraphicsInformationThread;
    pthread_create(&getGraphicsInformationThread, nullptr, RDCStreamingClient::GetGraphicsInformationThreadFunc, nullptr);

    pthread_t graphicsHandlingThread;
    pthread_create(&graphicsHandlingThread, nullptr, RDCStreamingClient::GraphicsHandlingThreadFunc, nullptr);
    pthread_detach(graphicsHandlingThread);

    RDCStreamingClient::isRunning = true;

    return true;
}

bool RDCStreamingClient::Stop()
{
    if (!RDCStreamingClient::isRunning)
        return false;

    close(RDCStreamingClient::serverSocket);

    RDCStreamingClient::isRunning = false;

    return true;
}

bool RDCStreamingClient::InitializeGraphics()
{
    socklen_t serverSocketAddrLength = sizeof(RDCStreamingClient::serverSocketAddr);

    bool connectionRequest = true;
    while (sendto(RDCStreamingClient::serverSocket, &connectionRequest, sizeof(connectionRequest), 0, (struct sockaddr *)&RDCStreamingClient::serverSocketAddr, serverSocketAddrLength) <= 0);

    while (recvfrom(RDCStreamingClient::serverSocket, &RDCStreamingClient::windowHeight, sizeof(int), 0, (struct sockaddr *)&RDCStreamingClient::serverSocketAddr, &serverSocketAddrLength) <= 0);
    while (recvfrom(RDCStreamingClient::serverSocket, &RDCStreamingClient::windowWidth, sizeof(int), 0, (struct sockaddr *)&RDCStreamingClient::serverSocketAddr, &serverSocketAddrLength) <= 0);

    Display * defaultDisplay = XOpenDisplay(nullptr);
    if (defaultDisplay == nullptr)
        return false;

    RDCStreamingClient::screenColors = new unsigned long *[RDCStreamingClient::windowHeight];
    for (int screenColorsY = 0; screenColorsY < RDCStreamingClient::windowHeight; ++screenColorsY)
        RDCStreamingClient::screenColors[screenColorsY] = new unsigned long[RDCStreamingClient::windowWidth];
    
    int defaultScreen = DefaultScreen(defaultDisplay);
    Window rootWindow = RootWindow(defaultDisplay, defaultScreen);

    RDCStreamingClient::graphicsWindow = XCreateSimpleWindow(defaultDisplay, rootWindow, 0, 0, RDCStreamingClient::windowWidth, RDCStreamingClient::windowHeight, 0, BlackPixel(defaultDisplay, defaultScreen),
        WhitePixel(defaultDisplay, defaultScreen));
    XSelectInput(defaultDisplay, RDCStreamingClient::graphicsWindow, ExposureMask | KeyPressMask);
    XMapWindow(defaultDisplay, RDCStreamingClient::graphicsWindow);

    return true;
}

void * RDCStreamingClient::GetGraphicsInformationThreadFunc(void * threadArguments)
{
    socklen_t serverSocketAddrLength = sizeof(RDCStreamingClient::serverSocketAddr);

    int receivedBytes;
    bool serverRunning;
    string receivedBuffer;
    istringstream receivedBufferStream;
    char receivedBufferCString[SOCKET_BUFFER_LENGTH];
    int xCoord, yCoord;
    while (RDCStreamingClient::isRunning)
    {
        receivedBytes = (recvfrom(RDCStreamingClient::serverSocket, receivedBufferCString, SOCKET_BUFFER_LENGTH, MSG_DONTWAIT, (struct sockaddr *)&RDCStreamingClient::serverSocketAddr, &serverSocketAddrLength));
        
        bool serverRunning = (receivedBytes != 0);
        if (!serverRunning)
        {
            RDCStreamingClient::Stop();
            return nullptr;
        }
        else if (receivedBytes < 0)
            continue;

        receivedBufferCString[receivedBytes] = '\0';
        receivedBuffer = receivedBufferCString;

        receivedBufferStream = istringstream(receivedBuffer);
        receivedBufferStream>>yCoord>>xCoord;
        receivedBufferStream>>RDCStreamingClient::screenColors[yCoord][xCoord];
    }

    return nullptr;
}

void * RDCStreamingClient::GraphicsHandlingThreadFunc(void * threadArguments)
{
    Display * defaultDisplay = XOpenDisplay(nullptr);

    XEvent thrownEvent;
    XGCValues pointGCValue;
    GC pointGC;
    XColor xColor;
    while (RDCStreamingClient::isRunning)
    {
        XNextEvent(defaultDisplay, &thrownEvent);

        switch (thrownEvent.type)
        {
            case Expose:
            {
                for (int colorArrayY = 0; colorArrayY < RDCStreamingClient::windowHeight; ++colorArrayY)
                    for (int colorArrayX = 0; colorArrayX < RDCStreamingClient::windowWidth; ++colorArrayX)
                    {
                        pointGCValue.function = GXcopy;
                        pointGCValue.plane_mask = AllPlanes;
                        pointGCValue.foreground = RDCStreamingClient::screenColors[colorArrayY][colorArrayX];
                        pointGCValue.background = 0xFFFFFF;

                        pointGC = XCreateGC(defaultDisplay, RDCStreamingClient::graphicsWindow, GCFunction|GCPlaneMask|GCForeground|GCBackground, &pointGCValue);

                        XDrawPoint(defaultDisplay, RDCStreamingClient::graphicsWindow, pointGC, colorArrayX, colorArrayY);
                    }

                continue;
            }
        }
    }

    return nullptr;
}

string RDCStreamingClient::serverIP_Get()
{
    return RDCStreamingClient::serverIP;
}

unsigned int RDCStreamingClient::serverPort_Get()
{
    return RDCStreamingClient::serverPort;
}

bool RDCStreamingClient::isRunning_Get()
{
    return RDCStreamingClient::isRunning;
}