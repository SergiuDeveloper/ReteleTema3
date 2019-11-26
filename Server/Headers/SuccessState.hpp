#ifndef SUCCESS_STATE_HPP
    #define SUCCESS_STATE_HPP
#endif

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