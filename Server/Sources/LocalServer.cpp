#include "../Headers/LocalServer.hpp"

LocalServer * LocalServer::singletonInstance;
pthread_mutex_t LocalServer::singletonInstanceMutex;

SuccessState LocalServer::Start()
{
    if (this->serverRunning_Get())
        return SuccessState(false, ERROR_SERVER_ALREADY_RUNNING);

    //return Server::Start(/* server location gotten from db */);
}

SuccessState LocalServer::Stop()
{
    SuccessState successState = Server::Stop();
    if (!successState.isSuccess_Get())
        return successState;

    while (!pthread_mutex_trylock(&LocalServer::singletonInstanceMutex));
    delete LocalServer::singletonInstance;
    pthread_mutex_unlock(&LocalServer::singletonInstanceMutex);

    pthread_mutex_destroy(&LocalServer::singletonInstanceMutex);

    return successState;
}

void LocalServer::ClientConnected_EventCallback(ClientSocket clientSocket)
{
}

const LocalServer * LocalServer::GetSingletonInstance()
{
    int mutexLockReturnValue = pthread_mutex_lock(&LocalServer::singletonInstanceMutex);
    bool isMutexInitialized = (mutexLockReturnValue != EINVAL);
    bool lockSuccess = (mutexLockReturnValue == 0);

    if (!isMutexInitialized)
        pthread_mutex_init(&LocalServer::singletonInstanceMutex, nullptr);

    if (!lockSuccess)
        while (!pthread_mutex_trylock(&LocalServer::singletonInstanceMutex));

    if (LocalServer::singletonInstance == nullptr)
        LocalServer::singletonInstance = new LocalServer();

    pthread_mutex_unlock(&LocalServer::singletonInstanceMutex);

    return LocalServer::singletonInstance;
}