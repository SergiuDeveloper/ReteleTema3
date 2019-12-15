#include "../Headers/RDCStreamingServer.hpp"

int RDCStreamingServer::serverSocket;
unsigned int RDCStreamingServer::serverPort;
bool RDCStreamingServer::isRunning = false;
vector<string> whitelistedIPsVector;
pthread_mutex_t whitelistedIPsVectorMutex;
Display * RDCStreamingServer::serverDisplay;

bool RDCStreamingServer::Start()
{
    if (RDCStreamingServer::isRunning)
        return false;

    RDCStreamingServer::serverDisplay = XOpenDisplay(nullptr);
    if (RDCStreamingServer::serverDisplay == nullptr)
        return false;

    bool operationResult;

    operationResult = (RDCStreamingServer::serverSocket = socket(AF_INET, SOCK_DGRAM, 0) != -1);
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
    RDCStreamingServer::whitelistedClientsVector.clear();

    pthread_mutex_init(&RDCStreamingServer::whitelistedIPsVectorMutex, nullptr);

    GatherDisplayInfo();

    RDCStreamingServer::isRunning = true;

    return true;
}

bool RDCStreamingServer::Stop()
{
    if (!RDCStreamingServer::isRunning)
        return false;

    RDCStreamingServer::isRunning = false;
    delete RDCStreamingServer::serverDisplay;

    close(RDCStreamingServer::serverSocket);

    pthread_mutex_destroy(&RDCStreamingServer::whitelistedIPsVectorMutex);

    return true;
}

bool RDCStreamingServer::AddWhitelistedClient(int whitelistedClient)
{
    if (!RDCStreamingServer::isRunning)
        return false;

    for (auto & whitelistedIPEntry : RDCStreamingServer::whitelistedClientsVector)
        if (whitelistedIPEntry == whitelistedClient)
            return false;

    while (!pthread_mutex_trylock(&RDCStreamingServer::whitelistedIPsVectorMutex));
    RDCStreamingServer::whitelistedClientsVector.push_back(whitelistedClient);

    pthread_mutex_unlock(&RDCStreamingServer::whitelistedIPsVectorMutex);

    pthread_t streamDisplayThread;
    pthread_create(&streamDisplayThread, nullptr, RDCStreamingServer::StreamDisplayThreadFunc, nullptr);
    pthread_detach(streamDisplayThread);    

    return true;
}

void RDCStreamingServer::GatherDisplayInfo()
{
    Screen * defaultScreen = DefaultScreenOfDisplay(RDCStreamingServer::serverDisplay);

    RDCStreamingServer::colorArray = new XColor **[defaultScreen->height];
    for (int colorArrayY = 0; colorArrayY < defaultScreen->height; ++colorArrayY)
    {
        RDCStreamingServer::colorArray[colorArrayY] = new XColor *[defaultScreen->width];
        for (int colorArrayX = 0; colorArrayX < defaultScreen->width; ++colorArrayX)
            RDCStreamingServer::colorArray[colorArrayY][colorArrayX] = new XColor;
    }

    int rootWindow = defaultScreen->root;
    XImage * displayImage;
    int defaultColormap = defaultScreen->cmap;

    while (RDCStreamingServer::isRunning)
    {
        displayImage = XGetImage(RDCStreamingServer::serverDisplay, rootWindow, 0, 0, defaultScreen->width, defaultScreen->height, AllPlanes, XYPixmap);

        for (int colorArrayY = 0; colorArrayY < defaultScreen->height; ++colorArrayY)
            for (int colorArrayX = 0; colorArrayX < defaultScreen->width; ++colorArrayX)
            {
                RDCStreamingServer::colorArray[colorArrayY][colorArrayX]->pixel = XGetPixel(displayImage, colorArrayX, colorArrayY);
                XQueryColor(RDCStreamingServer::serverDisplay, defaultColormap, RDCStreamingServer::colorArray[colorArrayY][colorArrayX]);
            }
    }

    for (int colorArrayY = 0; colorArrayY < defaultScreen->height; ++colorArrayY)
    {
        for (int colorArrayX = 0; colorArrayX < defaultScreen->width; ++colorArrayX)
            delete RDCStreamingServer::colorArray[colorArrayY][colorArrayX];
        delete RDCStreamingServer::colorArray[colorArrayY];
    }
    delete RDCStreamingServer::colorArray;

    delete defaultScreen;
    delete displayImage;
}

void * RDCStreamingServer::StreamDisplayThreadFunc(void * threadArguments)
{
    while (RDCStreamingServer::isRunning)
    {
        send multiple times
        read once to confirm
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