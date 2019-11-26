#include "../Headers/SuccessState.hpp"

SuccessState::SuccessState(bool isSuccess, string successStateMessage) : isSuccess(isSuccess), successStateMessage(successStateMessage)
{
}

bool SuccessState::isSuccess_Get()
{
    return this->isSuccess;
}

string SuccessState::successStateMessage_Get()
{
    return this->successStateMessage;
}