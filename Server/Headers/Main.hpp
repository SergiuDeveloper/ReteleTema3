#pragma once

#define COMMAND_START_SERVER                "START"
#define COMMAND_STOP_SERVER                 "STOP"
#define ERROR_NOT_ROOT                      "You must run the program as the root superuser"
#define ERROR_FAILED_PIPE                   "Failed to create pipe for server communication"
#define ERROR_FAILED_FORK                   "Failed to create server process"
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
    private:  static bool StartServer();
    private:  static bool StopServer();

    private:  static void ProgramExit_EventCallback();

    private: static vector<string> CastCommandLineArguments(int argumentsCount, char ** argumentsArray, bool ignoreApplicationPath);
};