#include "../Headers/SuccessState.hpp"

SuccessState::SuccessState(bool isSuccess, string successStateMessage) : isSuccess(isSuccess), successStateMessage(successStateMessage)
{
}

const bool SuccessState::isSuccess_Get()
{
    return this->isSuccess;
}

const string SuccessState::successStateMessage_Get()
{
    return this->successStateMessage;
}