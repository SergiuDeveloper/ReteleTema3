#include "../Headers/RDCStreamingServer.hpp"

int RDCStreamingServer::serverSocket;
unsigned int RDCStreamingServer::serverPort;
bool RDCStreamingServer::isRunning = false;
vector<string> whitelistedIPsVector;
pthread_mutex_t whitelistedIPsVectorMutex;
Display * RDCStreamingServer::serverDisplay;
unsigned long ** RDCStreamingServer::colorArray;
vector<string> RDCStreamingServer::colorArraySerialized;

bool RDCStreamingServer::IsGraphicsCompatible()
{
    Display * xDisplay = XOpenDisplay(nullptr);
    bool isGraphicsCompatible = (xDisplay != nullptr);

    return isGraphicsCompatible;
}

bool RDCStreamingServer::Start()
{
    if (RDCStreamingServer::isRunning)
        return false;

    RDCStreamingServer::serverDisplay = XOpenDisplay(nullptr);
    if (RDCStreamingServer::serverDisplay == nullptr)
        return false;

    bool operationResult;
 
    operationResult = ((RDCStreamingServer::serverSocket = socket(AF_INET, SOCK_DGRAM, 0)) != -1);
    if (!operationResult)
        return false;

    struct sockaddr_in serverSocketAddr;
    serverSocketAddr.sin_family = AF_INET;
    serverSocketAddr.sin_addr.s_addr = INADDR_ANY;
    serverSocketAddr.sin_port = htons(0);

    operationResult = (bind(RDCStreamingServer::serverSocket, (struct sockaddr *)&serverSocketAddr, sizeof(serverSocketAddr)) != -1);
    if (!operationResult)
        return false;

    socklen_t serverSocketAddrLength = sizeof(serverSocketAddr);
    getsockname(RDCStreamingServer::serverSocket, (struct sockaddr *)&serverSocketAddr, &serverSocketAddrLength);

    RDCStreamingServer::serverPort = ntohs(serverSocketAddr.sin_port);

    RDCStreamingServer::isRunning = true;

    pthread_t gatherDisplayInfoThread;
    pthread_create(&gatherDisplayInfoThread, nullptr, RDCStreamingServer::GatherDisplayInfoThreadFunc, nullptr);
    pthread_detach(gatherDisplayInfoThread);

    return true;
}

bool RDCStreamingServer::Stop()
{
    if (!RDCStreamingServer::isRunning)
        return false;

    RDCStreamingServer::isRunning = false;

    close(RDCStreamingServer::serverSocket);

    return true;
}

bool RDCStreamingServer::AddWhitelistedClient(struct sockaddr_in clientSocketAddr)
{
    if (!RDCStreamingServer::isRunning)
        return false;

    pthread_t streamDisplayThread;
    pthread_create(&streamDisplayThread, nullptr, RDCStreamingServer::StreamDisplayThreadFunc, &clientSocketAddr);
    pthread_detach(streamDisplayThread);    

    return true;
}

void * RDCStreamingServer::GatherDisplayInfoThreadFunc(void * threadArguments)
{
    Screen * defaultScreen = DefaultScreenOfDisplay(RDCStreamingServer::serverDisplay);

    RDCStreamingServer::colorArray = new unsigned long *[defaultScreen->height];
    for (int colorArrayY = 0; colorArrayY < defaultScreen->height; ++colorArrayY)
        RDCStreamingServer::colorArray[colorArrayY] = new unsigned long[defaultScreen->width];

    int rootWindow = defaultScreen->root;
    XImage * displayImage;
    int defaultColormap = defaultScreen->cmap;

    while (RDCStreamingServer::isRunning)
    {
        displayImage = XGetImage(RDCStreamingServer::serverDisplay, rootWindow, 0, 0, defaultScreen->width, defaultScreen->height, AllPlanes, XYPixmap);

        for (int colorArrayY = 0; colorArrayY < defaultScreen->height; ++colorArrayY)
            for (int colorArrayX = 0; colorArrayX < defaultScreen->width; ++colorArrayX)
                RDCStreamingServer::colorArray[colorArrayY][colorArrayX] = XGetPixel(displayImage, colorArrayX, colorArrayY);

        RDCStreamingServer::SerializeColorArray(defaultScreen->height, defaultScreen->width);
    }

    for (int colorArrayY = 0; colorArrayY < defaultScreen->height; ++colorArrayY)
        delete RDCStreamingServer::colorArray[colorArrayY];
    delete RDCStreamingServer::colorArray;

    delete defaultScreen;
    delete displayImage;

    return nullptr;
}

void RDCStreamingServer::SerializeColorArray(int screenHeight, int screenWidth)
{
    vector<string> serializedColorArray;
    
    string vectorEntry = "";
    string stringToAdd = "";
    for (int colorArrayY = 0; colorArrayY < screenHeight; ++colorArrayY)
        for (int colorArrayX = 0; colorArrayX < screenWidth; ++colorArrayX)
        {
            stringToAdd = to_string(colorArrayY) + ' ' + to_string(colorArrayX) + ' ' + to_string(RDCStreamingServer::colorArray[colorArrayY][colorArrayX]);

            if (vectorEntry.size() + stringToAdd.size() <= SOCKET_BUFFER_LENGTH)
                vectorEntry += stringToAdd;
            else
            {
                serializedColorArray.push_back(vectorEntry);
                vectorEntry = "";
            }
        }

    RDCStreamingServer::colorArraySerialized = serializedColorArray;
}

void * RDCStreamingServer::StreamDisplayThreadFunc(void * threadArguments)
{
    struct sockaddr_in clientSocketAddr = * ((struct sockaddr_in *)threadArguments);

    if (RDCStreamingServer::isRunning)
    {
        Screen * defaultScreen = DefaultScreenOfDisplay(RDCStreamingServer::serverDisplay);

        int screenHeight = defaultScreen->height;
        int screenWidth = defaultScreen->width;

        delete defaultScreen;

        sendto(RDCStreamingServer::serverSocket, &screenHeight, sizeof(int), MSG_CONFIRM, (struct sockaddr *)&clientSocketAddr, sizeof(clientSocketAddr));
        sendto(RDCStreamingServer::serverSocket, &screenWidth, sizeof(int), MSG_CONFIRM, (struct sockaddr *)&clientSocketAddr, sizeof(clientSocketAddr));
    }

    bool clientResponse;
    socklen_t clientSocketAddrLength;
    bool clientActive = true;

    while (RDCStreamingServer::isRunning && clientActive)
    {
        clientSocketAddrLength = sizeof(clientSocketAddr);

        for (auto & serializedColorArrayEntry : RDCStreamingServer::colorArraySerialized)
            sendto(RDCStreamingServer::serverSocket, serializedColorArrayEntry.c_str(), serializedColorArrayEntry.size(), MSG_DONTWAIT, (struct sockaddr *)&clientSocketAddr, clientSocketAddrLength);

        clientActive = (recvfrom(RDCStreamingServer::serverSocket, &clientResponse, sizeof(clientResponse), MSG_DONTWAIT, (struct sockaddr *)&clientSocketAddr, &clientSocketAddrLength) != 0);
    }

    return nullptr;
}

const unsigned int RDCStreamingServer::serverPort_Get()
{
    return RDCStreamingServer::serverPort;
}

const bool RDCStreamingServer::isRunning_Get()
{
    return RDCStreamingServer::isRunning;
}