#define ERROR_COMMAND_LINE_ARGUMENTS_COUNT "One command-line argument must be provided : <server port>"
#define ERROR_NOT_ROOT                     "You must run the program as the root superuser"

#include <iostream>
#include "Headers/SpecializedServer.hpp"

using namespace std;

void ProgramExit_EventCallback()
{
    if (!SpecializedServer::serverRunning_Get())
        return;

    SuccessState successState = SpecializedServer::Stop();

    cout<<successState.successStateMessage_Get()<<'\n';
}

int main(int argc, char ** argv)
{
    if (argc != 2)
    {
        cout<<ERROR_COMMAND_LINE_ARGUMENTS_COUNT<<'\n';
        return EXIT_FAILURE;
    }
    if (geteuid() != 0)
    {
        cout<<ERROR_NOT_ROOT<<'\n';
        return EXIT_FAILURE;
    }

    unsigned int serverPort = atoi(argv[1]);

    atexit(ProgramExit_EventCallback);
    at_quick_exit(ProgramExit_EventCallback);

    SuccessState successState = SpecializedServer::Start(serverPort);
    cout<<successState.successStateMessage_Get()<<'\n';
    
    while (SpecializedServer::serverRunning_Get());

    return (successState.isSuccess_Get() ? EXIT_SUCCESS : EXIT_FAILURE);
}