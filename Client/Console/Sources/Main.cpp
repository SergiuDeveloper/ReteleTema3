#include "../Headers/Main.hpp"

bool Main::Initialize(int argumentsCount, char ** argumentsArray)
{
    if (geteuid() != 0)
    {
        cout<<ERROR_NOT_ROOT<<endl;
        return false;
    }

    vector<string> argumentsList = Main::CastCommandLineArguments(argumentsCount, argumentsArray, true);

    if (argumentsList.size() == 0)
    {
        cout<<ERROR_COMMAND_LINE_ARGUMENTS<<endl;
        return false;
    }

    at_quick_exit(Main::ProgramExit_EventCallback);

    string serverIP;
    unsigned int serverPort;

    if (argumentsList.size() == 1)
    {
        size_t delimiterPosition = argumentsList[0].find(':');
        
        if (delimiterPosition == string::npos)
        {
            cout<<ERROR_COMMAND_LINE_ARGUMENTS<<endl;
            return false;
        }

        serverIP = argumentsList[0].substr(0, delimiterPosition);
        serverPort = atoi(argumentsList[0].erase(0, delimiterPosition + 1).c_str());
    }
    else
    {
        serverIP = argumentsList[0];
        serverPort = atoi(argumentsList[1].c_str());
    }

    Client clientInstance = * Client::GetSingletonInstance();
    SuccessState successState = clientInstance.Connect(serverIP, serverPort);

    cout<<successState.successStateMessage_Get()<<endl;

    return successState.isSuccess_Get();
}

void Main::ProgramExit_EventCallback()
{
    Client clientInstance = * (Client::GetSingletonInstance());
    
    if (!clientInstance.isConnected_Get())
        return;

    SuccessState successState = clientInstance.Disconnect();

    cout<<successState.successStateMessage_Get()<<endl;
}

vector<string> Main::CastCommandLineArguments(int argumentsCount, char ** argumentsArray, bool ignoreApplicationPath)
{
    vector<string> argumentsList;
    for (int argumentsArrayIterator = (ignoreApplicationPath ? 1 : 0); argumentsArrayIterator < argumentsCount; ++argumentsArrayIterator)
        argumentsList.push_back(argumentsArray[argumentsArrayIterator]);

    return argumentsList;
}