#include "../Headers/RDCStreamingClient.hpp"

string RDCStreamingClient::serverIP;
unsigned int RDCStreamingClient::serverPort;
int RDCStreamingClient::serverSocket;
struct sockaddr_in RDCStreamingClient::serverSocketAddr;
bool RDCStreamingClient::isRunning = false;
Display * RDCStreamingClient::RDCStreamingClient::defaultDisplay;
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

    RDCStreamingClient::serverIP = serverIP;
    RDCStreamingClient::serverPort = serverPort;

    RDCStreamingClient::isRunning = true;

    pthread_t getGraphicsInformationThread;
    pthread_create(&getGraphicsInformationThread, nullptr, RDCStreamingClient::GetGraphicsInformationThreadFunc, nullptr);
    pthread_detach(getGraphicsInformationThread);

    pthread_t graphicsHandlingThread;
    pthread_create(&graphicsHandlingThread, nullptr, RDCStreamingClient::GraphicsHandlingThreadFunc, nullptr);
    pthread_detach(graphicsHandlingThread);

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

    RDCStreamingClient::defaultDisplay = XOpenDisplay(nullptr);
    if (RDCStreamingClient::defaultDisplay == nullptr)
        return false;

    XInitThreads();

    RDCStreamingClient::screenColors = new unsigned long *[RDCStreamingClient::windowHeight];
    for (int screenColorsY = 0; screenColorsY < RDCStreamingClient::windowHeight; ++screenColorsY)
        RDCStreamingClient::screenColors[screenColorsY] = new unsigned long[RDCStreamingClient::windowWidth];
    
    int defaultScreen = DefaultScreen(RDCStreamingClient::defaultDisplay);
    Window rootWindow = RootWindow(RDCStreamingClient::defaultDisplay, defaultScreen);

    RDCStreamingClient::graphicsWindow = XCreateSimpleWindow(RDCStreamingClient::defaultDisplay, rootWindow, 0, 0, RDCStreamingClient::windowWidth, RDCStreamingClient::windowHeight, 0, BlackPixel(RDCStreamingClient::defaultDisplay, defaultScreen),
        WhitePixel(RDCStreamingClient::defaultDisplay, defaultScreen));
    XSelectInput(RDCStreamingClient::defaultDisplay, RDCStreamingClient::graphicsWindow, ExposureMask);
    XMapWindow(RDCStreamingClient::defaultDisplay, RDCStreamingClient::graphicsWindow);

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
    unsigned long receivedPixel;
    while (RDCStreamingClient::isRunning)
        if (RDCStreamingClient::screenColors != nullptr)
        {
            receivedBytes = (recvfrom(RDCStreamingClient::serverSocket, receivedBufferCString, SOCKET_BUFFER_LENGTH, MSG_DONTWAIT, (struct sockaddr *)&RDCStreamingClient::serverSocketAddr, &serverSocketAddrLength));
            
            serverRunning = (receivedBytes != 0);
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
            while (receivedBufferStream>>yCoord>>xCoord>>receivedPixel)
                RDCStreamingClient::screenColors[yCoord][xCoord] = receivedPixel;
        }

    return nullptr;
}

void * RDCStreamingClient::GraphicsHandlingThreadFunc(void * threadArguments)
{
    XGCValues pointGCValue;
    pointGCValue.cap_style = CapButt;
    pointGCValue.join_style = JoinBevel;

    GC pointGC = XCreateGC(RDCStreamingClient::defaultDisplay, RDCStreamingClient::graphicsWindow, GCCapStyle | GCJoinStyle, &pointGCValue);
    XSetFillStyle(RDCStreamingClient::defaultDisplay, pointGC, FillSolid);

    XEvent thrownEvent;
    unsigned long lastPixel;
    do
    {
        XNextEvent(RDCStreamingClient::defaultDisplay, &thrownEvent);
    }
    while (RDCStreamingClient::isRunning && thrownEvent.type != Expose);

    unsigned long ** pixelsCopy = new unsigned long *[RDCStreamingClient::windowHeight];
    for (int colorArrayY = 0; colorArrayY < RDCStreamingClient::windowHeight; ++colorArrayY)
        pixelsCopy[colorArrayY] = new unsigned long[RDCStreamingClient::windowWidth];

    bool initialIteration = true;
    while (RDCStreamingClient::isRunning)
    {
        for (int colorArrayY = 0; colorArrayY < RDCStreamingClient::windowHeight; ++colorArrayY)
            for (int colorArrayX = 0; colorArrayX < RDCStreamingClient::windowWidth; ++colorArrayX)
            {
                if (RDCStreamingClient::screenColors[colorArrayY][colorArrayX] != lastPixel)
                {
                    lastPixel = RDCStreamingClient::screenColors[colorArrayY][colorArrayX];
                    XSetForeground(RDCStreamingClient::defaultDisplay, pointGC, lastPixel);
                }
                
                if (initialIteration || RDCStreamingClient::screenColors[colorArrayY][colorArrayX] != pixelsCopy[colorArrayY][colorArrayX])
                {
                    XDrawPoint(RDCStreamingClient::defaultDisplay, RDCStreamingClient::graphicsWindow, pointGC, colorArrayX, colorArrayY);
                    pixelsCopy[colorArrayY][colorArrayX] = RDCStreamingClient::screenColors[colorArrayY][colorArrayX];
                }
            }

        XFlush(RDCStreamingClient::defaultDisplay);

        initialIteration = false;
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