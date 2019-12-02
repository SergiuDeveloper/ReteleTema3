#include "../Headers/Main.hpp"

bool Main::Initialize(int argumentsCount, char ** argumentsArray)
{
    if (geteuid() != 0)
    {
        cout<<ERROR_NOT_ROOT<<endl;
        return false;
    }

    vector<string> argumentsList = Main::CastCommandLineArguments(argumentsCount, argumentsArray, true);

    for (auto & iteratedArgument : argumentsList)
        for (auto & iteratedArgumentIterator : iteratedArgument)
            iteratedArgumentIterator = toupper(iteratedArgumentIterator);

    return Main::ExecuteTask(argumentsList);
}

bool Main::ExecuteTask(vector<string> argumentsList)
{
    if (argumentsList[0] == COMMAND_START_SERVER)
        return Main::StartServer();
    if (argumentsList[0] == COMMAND_STOP_SERVER)
        return Main::StopServer();

    return false;
}

bool Main::StartServer()
{
    bool operationSuccess;

    int serverPipe[2];
    operationSuccess = (pipe(serverPipe) >= 0);
    if (!operationSuccess)
    {
        cout<<ERROR_FAILED_PIPE<<endl;
        return false;
    }

    pid_t forkResult = fork();
    operationSuccess = (forkResult >= 0);
    if (!operationSuccess)
    {
        cout<<ERROR_FAILED_FORK<<endl;
        return false;
    }

    switch (forkResult)
    {
        case CHILD_PROCESS_FORK_RESULT:
        {
            close(serverPipe[PIPE_READ_INDEX]);
            
            atexit(Main::ProgramExit_EventCallback);
            at_quick_exit(Main::ProgramExit_EventCallback);

            SpecializedServer specializedServer = * (SpecializedServer::GetSingletonInstance());

            SuccessState successState = specializedServer.Start();
            
            bool successStateResult = successState.isSuccess_Get();
            write(serverPipe[PIPE_WRITE_INDEX], &successStateResult, sizeof(successStateResult));
            close(serverPipe[PIPE_WRITE_INDEX]);

            cout<<successState.successStateMessage_Get()<<endl;

            while (specializedServer.serverRunning_Get());

            return successStateResult ? true : false;
        }
        default:
        {
            close(serverPipe[PIPE_WRITE_INDEX]);

            bool successStateResult;
            read(serverPipe[PIPE_READ_INDEX], &successStateResult, sizeof(successStateResult));
            close(serverPipe[PIPE_READ_INDEX]);

            return successStateResult ? true : false;;
        }
    }

    return false;
}

bool Main::StopServer()
{
    SuccessState successState = LocalClient::StopServer();

    cout<<successState.successStateMessage_Get();

    return successState.isSuccess_Get();
}

void Main::ProgramExit_EventCallback()
{
    SpecializedServer specializedServer = * (SpecializedServer::GetSingletonInstance());
    
    if (!specializedServer.serverRunning_Get())
        return;

    SuccessState successState = specializedServer.Stop();

    cout<<successState.successStateMessage_Get()<<endl;
}

vector<string> Main::CastCommandLineArguments(int argumentsCount, char ** argumentsArray, bool ignoreApplicationPath)
{
    vector<string> argumentsList;
    for (int argumentsArrayIterator = (ignoreApplicationPath ? 1 : 0); argumentsArrayIterator < argumentsCount; ++argumentsArrayIterator)
        argumentsList.push_back(argumentsArray[argumentsArrayIterator]);

    return argumentsList;
}