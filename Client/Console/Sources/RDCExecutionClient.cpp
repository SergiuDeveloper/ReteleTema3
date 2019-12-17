#include "../Headers/RDCExecutionClient.hpp"

string RDCExecutionClient::serverIP;
unsigned int RDCExecutionClient::serverPort;
int RDCExecutionClient::serverSocket;
bool RDCExecutionClient::isRunning = false;
char * RDCExecutionClient::previousKeyboardState = nullptr;
char * RDCExecutionClient::keyboardState = nullptr;
size_t RDCExecutionClient::keyboardStateLength;
pthread_mutex_t RDCExecutionClient::keyboardStateMutex;
Window RDCExecutionClient::graphicsWindow;

bool RDCExecutionClient::IsGraphicsCompatible()
{
    Display * xDisplay = XOpenDisplay(nullptr);
    bool isGraphicsCompatible = (xDisplay != nullptr);

    return isGraphicsCompatible;
}

bool RDCExecutionClient::Start(string serverIP, unsigned int serverPort, Window graphicsWindow)
{
    if (RDCExecutionClient::isRunning)
        return false;

    if (!RDCExecutionClient::IsGraphicsCompatible())
        return false;

    RDCExecutionClient::serverIP = serverIP;
    RDCExecutionClient::serverPort = serverPort;

    bool operationSuccess = ((RDCExecutionClient::serverSocket = socket(AF_INET, SOCK_STREAM, 0)) != -1);
    if (!operationSuccess)
        return false;

    struct sockaddr_in serverSocketAddr;
    serverSocketAddr.sin_family = AF_INET;
    inet_pton(AF_INET, serverIP.c_str(), &serverSocketAddr.sin_addr);
    serverSocketAddr.sin_port = htons(serverPort);

    operationSuccess = (connect(RDCExecutionClient::serverSocket, (struct sockaddr *)&serverSocketAddr, sizeof(serverSocketAddr)) != -1);
    if (!operationSuccess)
        return false;

    RDCExecutionClient::graphicsWindow = graphicsWindow;

    pthread_mutex_init(&RDCExecutionClient::keyboardStateMutex, nullptr);

    RDCExecutionClient::isRunning = true;

    pthread_t receiveActionsThread;
    pthread_create(&receiveActionsThread, nullptr, RDCExecutionClient::ReceiveActionsThreadFunc, nullptr);
    pthread_detach(receiveActionsThread);

    pthread_t executeActionsThread;
    pthread_create(&executeActionsThread, nullptr, RDCExecutionClient::ExecuteActionsThreadFunc, nullptr);
    pthread_detach(executeActionsThread);

    return true;
}

bool RDCExecutionClient::Stop()
{
    if (!RDCExecutionClient::isRunning)
        return false;

    close(RDCExecutionClient::serverSocket);

    if (RDCExecutionClient::previousKeyboardState != nullptr)
        delete RDCExecutionClient::previousKeyboardState;
    if (RDCExecutionClient::keyboardState != nullptr)
        delete RDCExecutionClient::keyboardState;
    pthread_mutex_destroy(&RDCExecutionClient::keyboardStateMutex);

    RDCExecutionClient::isRunning = false;

    return true;
}

void * RDCExecutionClient::ReceiveActionsThreadFunc(void * threadArguments)
{
    if (!RDCExecutionClient::isRunning)
        return nullptr;

    bool aliveConfirmation = true;
    bool serverConnected = true;

    serverConnected = (read(RDCExecutionClient::serverSocket, &RDCExecutionClient::keyboardStateLength, sizeof(size_t)) != 0);
    
    while (RDCExecutionClient::isRunning && serverConnected)
    {
        while (!pthread_mutex_trylock(&RDCExecutionClient::keyboardStateMutex));
        if (RDCExecutionClient::keyboardState == nullptr)
            RDCExecutionClient::keyboardState = new char[RDCExecutionClient::keyboardStateLength];
        if (RDCExecutionClient::previousKeyboardState == nullptr)
            RDCExecutionClient::previousKeyboardState = new char[RDCExecutionClient::keyboardStateLength];

        serverConnected = (read(RDCExecutionClient::serverSocket, keyboardState, RDCExecutionClient::keyboardStateLength) != 0);
        pthread_mutex_unlock(&RDCExecutionClient::keyboardStateMutex);

        write(RDCExecutionClient::serverSocket, &aliveConfirmation, sizeof(aliveConfirmation));
    }

    RDCExecutionClient::Stop();

    return nullptr;
}

void * RDCExecutionClient::ExecuteActionsThreadFunc(void * threadArguments)
{
    Display * defaultDisplay = XOpenDisplay(nullptr);

    char * keyboardStateCopy = nullptr;
    char previousKeyboardStateFieldCopy;
    char keyboardStateCopyFieldCopy;
    size_t keyboardStateLengthCopy;
    Window focusedWindow;
    int focusState;
    int keyNumber;
    int currentKeyValue;
    int previousKeyValue;
    while (RDCExecutionClient::isRunning)
        if (RDCExecutionClient::keyboardState != nullptr)
        {
            XGetInputFocus(defaultDisplay, &focusedWindow, &focusState);

            if (focusedWindow != RDCExecutionClient::graphicsWindow)
                continue;

            if (keyboardStateCopy == nullptr)
            {
                keyboardStateCopy = new char[RDCExecutionClient::keyboardStateLength];
                keyboardStateLengthCopy = RDCExecutionClient::keyboardStateLength;
            }

            while (!pthread_mutex_trylock(&RDCExecutionClient::keyboardStateMutex));
            for (size_t keyboardStateIterator = 0; keyboardStateIterator < RDCExecutionClient::keyboardStateLength; ++keyboardStateIterator)
                keyboardStateCopy[keyboardStateIterator] = RDCExecutionClient::keyboardState[keyboardStateIterator];
            pthread_mutex_unlock(&RDCExecutionClient::keyboardStateMutex);

            for (size_t keyboardStateIterator = 0; keyboardStateIterator < keyboardStateLengthCopy; ++keyboardStateLengthCopy)
                if (RDCExecutionClient::previousKeyboardState[keyboardStateIterator] != keyboardStateCopy[keyboardStateIterator])
                {
                    previousKeyboardStateFieldCopy = RDCExecutionClient::previousKeyboardState[keyboardStateIterator];
                    keyboardStateCopyFieldCopy = keyboardStateCopy[keyboardStateIterator];

                    for (int byteIterator = 0; byteIterator < 8; ++byteIterator)
                    {
                        keyNumber = 8 * keyboardStateIterator + byteIterator;

                        currentKeyValue = (keyboardStateCopyFieldCopy & 0x1);
                        previousKeyValue = (previousKeyboardStateFieldCopy & 0x1);

                        if (previousKeyValue != currentKeyValue)
                            XTestFakeKeyEvent(defaultDisplay, keyNumber, currentKeyValue, CurrentTime);
                    
                        previousKeyboardStateFieldCopy = (previousKeyboardStateFieldCopy >> 1);
                        keyboardStateCopyFieldCopy = (keyboardStateCopyFieldCopy >> 1);
                    }

                    RDCExecutionClient::previousKeyboardState[keyboardStateIterator] = keyboardStateCopy[keyboardStateIterator];
                }
            XFlush(defaultDisplay);
        }

    if (keyboardStateCopy != nullptr)
        delete keyboardStateCopy;

    return nullptr;
}

string RDCExecutionClient::serverIP_Get()
{
    return RDCExecutionClient::serverIP;
}

unsigned int RDCExecutionClient::serverPort_Get()
{
    return RDCExecutionClient::serverPort;
}

bool RDCExecutionClient::isRunning_Get()
{
    return RDCExecutionClient::isRunning;
}