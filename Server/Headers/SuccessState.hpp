#pragma once

#include <string>

using namespace std;

class SuccessState
{
    public:  SuccessState(bool isSuccess, string successStateMessage = "");

    private: bool isSuccess;
    private: string successStateMessage;

    public:  bool isSuccess_Get();
    public:  string successStateMessage_Get();
};