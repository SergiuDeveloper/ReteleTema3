#pragma once

#include <string>

using namespace std;

class SuccessState
{
    public:  SuccessState(bool isSuccess, string successStateMessage = "");

    private: bool isSuccess;
    private: string successStateMessage;

    public:  const bool isSuccess_Get();
    public:  const string successStateMessage_Get();
};