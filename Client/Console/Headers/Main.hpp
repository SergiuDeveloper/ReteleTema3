#pragma once

#define ERROR_NOT_ROOT                      "You must run the program as the root superuser"
#define ERROR_COMMAND_LINE_ARGUMENTS        "You must provide a server IP and port"

#include <iostream>
#include <vector>
#include <string>
#include "Client.hpp"
#include "SuccessState.hpp"

using namespace std;

class Main
{
    public:   static bool Initialize(int argumentsCount, char ** argumentsArray);

    private:  static void ProgramExit_EventCallback();

    private:  static vector<string> CastCommandLineArguments(int argumentsCount, char ** argumentsArray, bool ignoreApplicationPath);
};