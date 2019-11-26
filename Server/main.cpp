#define ERROR_COMMAND_LINE_ARGUMENTS_COUNT "One command-line argument must be provided : <server port>"

#include <iostream>
#include "Headers/Server.hpp"

using namespace std;

void ProgramExit_EventCallback()
{
    if (!Server::serverRunning_Get())
        return;

    SuccessState successState = Server::Stop();

    cout<<successState.successStateMessage_Get()<<'\n';
}

int main(int argc, char ** argv)
{
    if (argc != 2)
    {
        cout<<ERROR_COMMAND_LINE_ARGUMENTS_COUNT<<'\n';
        return EXIT_FAILURE;
    }
    unsigned int serverPort = atoi(argv[1]);

    atexit(ProgramExit_EventCallback);
    at_quick_exit(ProgramExit_EventCallback);

    SuccessState successState = Server::Start(serverPort);

    cout<<successState.successStateMessage_Get()<<'\n';

    return (successState.isSuccess_Get() ? EXIT_SUCCESS : EXIT_FAILURE);
}