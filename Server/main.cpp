#define ERROR_COMMAND_LINE_ARGUMENTS_COUNT  "One command-line argument must be provided : <server port>"
#define ERROR_NOT_ROOT                      "You must run the program as the root superuser"
#define ERROR_FAILED_PIPE                   "Failed to create pipe for server communication"
#define ERROR_FAILED_FORK                   "Failed to create server process"
#define CHILD_PROCESS_FORK_RESULT           0
#define PIPE_READ_INDEX                     0
#define PIPE_WRITE_INDEX                    1

#include <iostream>
#include <string>
#include "Headers/SpecializedServer.hpp"

using namespace std;

void ProgramExit_EventCallback()
{
    SpecializedServer specializedServer = * (SpecializedServer::GetSingletonInstance());
    
    if (!specializedServer.serverRunning_Get())
        return;

    SuccessState successState = specializedServer.Stop();

    cout<<successState.successStateMessage_Get()<<endl;
}

int main(int argc, char ** argv)
{
    if (argc != 2)
    {
        cout<<ERROR_COMMAND_LINE_ARGUMENTS_COUNT<<endl;
        return EXIT_FAILURE;
    }
    if (geteuid() != 0)
    {
        cout<<ERROR_NOT_ROOT<<endl;
        return EXIT_FAILURE;
    }

    unsigned int serverPort = atoi(argv[1]);

    bool operationSuccess;

    int serverPipe[2];
    operationSuccess = (pipe(serverPipe) >= 0);
    if (!operationSuccess)
    {
        cout<<ERROR_FAILED_PIPE<<endl;
        return EXIT_FAILURE;
    }

    pid_t forkResult = fork();
    operationSuccess = (forkResult >= 0);
    if (!operationSuccess)
    {
        cout<<ERROR_FAILED_FORK<<endl;
        return EXIT_FAILURE;
    }

    switch (forkResult)
    {
        case CHILD_PROCESS_FORK_RESULT:
        {
            close(serverPipe[PIPE_READ_INDEX]);
            
            atexit(ProgramExit_EventCallback);
            at_quick_exit(ProgramExit_EventCallback);

            SpecializedServer specializedServer = * (SpecializedServer::GetSingletonInstance());

            SuccessState successState = specializedServer.Start();
            
            bool successStateResult = successState.isSuccess_Get();
            write(serverPipe[PIPE_WRITE_INDEX], &successStateResult, sizeof(successStateResult));
            close(serverPipe[PIPE_WRITE_INDEX]);

            cout<<successState.successStateMessage_Get()<<endl;

            while (specializedServer.serverRunning_Get());

            return successStateResult ? EXIT_SUCCESS : EXIT_FAILURE;
        }
        default:
        {
            close(serverPipe[PIPE_WRITE_INDEX]);

            bool successStateResult;
            read(serverPipe[PIPE_READ_INDEX], &successStateResult, sizeof(successStateResult));
            close(serverPipe[PIPE_READ_INDEX]);

            return successStateResult ? EXIT_SUCCESS : EXIT_FAILURE;;
        }
    }

    return EXIT_FAILURE;
}