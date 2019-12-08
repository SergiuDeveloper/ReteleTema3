#pragma once

#define COMMAND_START_SERVER                "START"
#define COMMAND_STOP_SERVER                 "STOP"
#define ERROR_COMMAND_LINE_ARGUMENTS_START  "You must provide a port for the server to bind"
#define ERROR_NOT_ROOT                      "You must run the program as the root superuser"
#define ERROR_FAILED_PIPE                   "Failed to create pipe for server communication"
#define ERROR_FAILED_FORK                   "Failed to create server process"
#define ERROR_NO_ARGUMENTS_PROVIDED         "No command line arguments provided"
#define ERROR_COMMAND_NOT_FOUND             "Failed to execute the requested task, as the command line arguments pattern is unknown"
#define CHILD_PROCESS_FORK_RESULT           0
#define PIPE_READ_INDEX                     0
#define PIPE_WRITE_INDEX                    1

#include <iostream>
#include <vector>
#include <string>
#include "SpecializedServer.hpp"
#include "LocalClient.hpp"

using namespace std;

class Main
{
    public:   static bool Initialize(int argumentsCount, char ** argumentsArray);

    private:  static bool ExecuteTask(vector<string> argumentsList);
    private:  static bool StartServer(vector<string> argumentsList);
    private:  static bool StopServer();

    private:  static void ProgramExit_EventCallback();

    private: static vector<string> CastCommandLineArguments(int argumentsCount, char ** argumentsArray, bool ignoreApplicationPath);
};