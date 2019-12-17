#include "../Headers/RDCStreamingServer.hpp"

int RDCStreamingServer::serverSocket;
unsigned int RDCStreamingServer::serverPort;
bool RDCStreamingServer::isRunning = false;
vector<string> RDCStreamingServer::whitelistedIPsVector;
pthread_mutex_t RDCStreamingServer::whitelistedIPsVectorMutex;
Display * RDCStreamingServer::serverDisplay;
unsigned long ** RDCStreamingServer::colorArray;
vector<string> RDCStreamingServer::colorArraySerialized;
pthread_mutex_t RDCStreamingServer::colorArraySerializedMutex;

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

    pthread_mutex_init(&RDCStreamingServer::whitelistedIPsVectorMutex, nullptr);
    pthread_mutex_init(&RDCStreamingServer::colorArraySerializedMutex, nullptr);

    RDCStreamingServer::whitelistedIPsVector.clear();

    pthread_t receiveConnectionsThread;
    pthread_create(&receiveConnectionsThread, nullptr, RDCStreamingServer::ReceiveConnectionsThreadFunc, nullptr);
    pthread_detach(receiveConnectionsThread);

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

    pthread_mutex_destroy(&RDCStreamingServer::whitelistedIPsVectorMutex);
    pthread_mutex_destroy(&RDCStreamingServer::colorArraySerializedMutex);

    close(RDCStreamingServer::serverSocket);

    return true;
}

bool RDCStreamingServer::AddWhitelistedClient(struct sockaddr_in clientSocketAddr)
{
    if (!RDCStreamingServer::isRunning)
        return false;

    while (!pthread_mutex_trylock(&RDCStreamingServer::whitelistedIPsVectorMutex));
    RDCStreamingServer::whitelistedIPsVector.push_back(inet_ntoa(clientSocketAddr.sin_addr));
    pthread_mutex_unlock(&RDCStreamingServer::whitelistedIPsVectorMutex);
    
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

    size_t whitelistedIPsVectorLength;
    while (RDCStreamingServer::isRunning)
    {
        while (!pthread_mutex_trylock(&RDCStreamingServer::whitelistedIPsVectorMutex));
        whitelistedIPsVectorLength = RDCStreamingServer::whitelistedIPsVector.size();
        pthread_mutex_unlock(&RDCStreamingServer::whitelistedIPsVectorMutex);

        if (whitelistedIPsVectorLength > 0)
        {
            displayImage = XGetImage(RDCStreamingServer::serverDisplay, rootWindow, 0, 0, defaultScreen->width, defaultScreen->height, AllPlanes, XYPixmap);

            for (int colorArrayY = 0; colorArrayY < defaultScreen->height; ++colorArrayY)
                for (int colorArrayX = 0; colorArrayX < defaultScreen->width; ++colorArrayX)
                    RDCStreamingServer::colorArray[colorArrayY][colorArrayX] = XGetPixel(displayImage, colorArrayX, colorArrayY);

            RDCStreamingServer::SerializeColorArray(defaultScreen->height, defaultScreen->width);
        }
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
            stringToAdd = to_string(colorArrayY) + ' ' + to_string(colorArrayX) + ' ' + to_string(RDCStreamingServer::colorArray[colorArrayY][colorArrayX]) + ' ';

            if (vectorEntry.size() + stringToAdd.size() <= SOCKET_BUFFER_LENGTH)
                vectorEntry += stringToAdd;
            else
            {
                serializedColorArray.push_back(vectorEntry);
                vectorEntry = "";
            }
        }

    while (!pthread_mutex_trylock(&RDCStreamingServer::colorArraySerializedMutex));
    RDCStreamingServer::colorArraySerialized = serializedColorArray;
    pthread_mutex_unlock(&RDCStreamingServer::colorArraySerializedMutex);
}

void * RDCStreamingServer::ReceiveConnectionsThreadFunc(void * threadArguments)
{
    struct sockaddr_in clientSocketAddr;
    socklen_t clientSocketAddrLength = sizeof(clientSocketAddr);

    bool connectionRequest;
    string clientIP;
    while (RDCStreamingServer::isRunning)
        if (recvfrom(RDCStreamingServer::serverSocket, &connectionRequest, sizeof(connectionRequest), 0, (struct sockaddr *)&clientSocketAddr, &clientSocketAddrLength) > 0)
        {
            clientIP = inet_ntoa(clientSocketAddr.sin_addr);
            
            for (auto & whitelistedIP : RDCStreamingServer::whitelistedIPsVector)
                if (whitelistedIP == clientIP)
                {
                    pthread_t streamDisplayThread;
                    pthread_create(&streamDisplayThread, nullptr, RDCStreamingServer::StreamDisplayThreadFunc, &clientSocketAddr);
                    pthread_detach(streamDisplayThread);

                    continue;
                }
        }

    return nullptr;
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

        while (sendto(RDCStreamingServer::serverSocket, &screenHeight, sizeof(int), 0, (struct sockaddr *)&clientSocketAddr, sizeof(clientSocketAddr)) <= 0);
        while (sendto(RDCStreamingServer::serverSocket, &screenWidth, sizeof(int), 0, (struct sockaddr *)&clientSocketAddr, sizeof(clientSocketAddr)) <= 0);
    }

    vector<string> colorArraySerializedCopy;
    bool receivedConfirmation;
    socklen_t clientSocketAddrLength = sizeof(clientSocketAddr);
    bool clientConnected = true;
    while (RDCStreamingServer::isRunning && clientConnected)
    {
        while (!pthread_mutex_trylock(&RDCStreamingServer::colorArraySerializedMutex));
        colorArraySerializedCopy = RDCStreamingServer::colorArraySerialized;
        pthread_mutex_unlock(&RDCStreamingServer::colorArraySerializedMutex);
        
        for (auto & serializedColorArrayEntry : colorArraySerializedCopy)
        {
            sendto(RDCStreamingServer::serverSocket, serializedColorArrayEntry.c_str(), SOCKET_BUFFER_LENGTH, 0, (struct sockaddr *)&clientSocketAddr, clientSocketAddrLength);
            
            clientConnected = (recvfrom(RDCStreamingServer::serverSocket, &receivedConfirmation, sizeof(receivedConfirmation), 0, (struct sockaddr *)&clientSocketAddr, &clientSocketAddrLength) != 0);
        }
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