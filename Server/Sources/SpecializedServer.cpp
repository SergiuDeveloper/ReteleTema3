#include "../Headers/SpecializedServer.hpp"

SpecializedServer * SpecializedServer::singletonInstance = nullptr;
pthread_mutex_t SpecializedServer::singletonInstanceMutex;

SuccessState SpecializedServer::Start()
{
    if (this->serverRunning_Get())
        return SuccessState(false, ERROR_SERVER_ALREADY_RUNNING);

    string mySQLSuperuserPassword = getpass(PROMPT_MYSQL_DB_CREDENTIALS);

    SuccessState successState = MySQLConnector::Initialize(mySQLSuperuserPassword);
    
    if (successState.isSuccess_Get())
        cout<<successState.successStateMessage_Get()<<endl;

    if (!successState.isSuccess_Get())
        return successState;
    
    pthread_mutex_init(&this->consoleMutex, nullptr);
    
    successState = Server::Start(HTTP_PORT);

    if (!successState.isSuccess_Get())
        return successState;

    SuccessState localServerSuccessState = localServer.Start();
    if (!localServerSuccessState.isSuccess_Get())
    {
        Server::Stop();
        
        return localServerSuccessState;
    }
    
    return successState;
}

SuccessState SpecializedServer::Stop()
{
    SuccessState successState = Server::Stop();
    if (!successState.isSuccess_Get())
        return successState;

    localServer.Stop();

    MySQLConnector::Deinitialize();

    pthread_mutex_destroy(&this->consoleMutex);

    while (!pthread_mutex_trylock(&SpecializedServer::singletonInstanceMutex));
    delete SpecializedServer::singletonInstance;
    pthread_mutex_unlock(&SpecializedServer::singletonInstanceMutex);

    pthread_mutex_destroy(&SpecializedServer::singletonInstanceMutex);

    return successState;
}

void SpecializedServer::ClientConnected_EventCallback(ClientSocket clientSocket)
{
    PreparedStatement * mySQLStatement;
    ResultSet * mySQLResultSet;
    bool isWhitelistedCient = false;

    try
    {
        Connection * mySQLConnection = MySQLConnector::mySQLConnection_Get();

        mySQLStatement = mySQLConnection->prepareStatement(MYSQL_IS_WHITELISTED_CLIENT_QUERY);
        mySQLStatement->setString(1, clientSocket.clientIP);
        mySQLStatement->setString(2, clientSocket.clientMAC);
        mySQLResultSet = mySQLStatement->executeQuery();

        if (mySQLResultSet->next())
            isWhitelistedCient = mySQLResultSet->getBoolean(1);
    }
    catch (SQLException & mySQLException)
    {
        cout<<ERROR_MYSQL_GENERIC_ERROR(mySQLException.getErrorCode(), mySQLException.what())<<endl;

        if (mySQLStatement != nullptr)
        {
            mySQLStatement->close();
            delete mySQLStatement;
        }
        if (mySQLResultSet != nullptr)
        {
            mySQLResultSet->close();
            delete mySQLResultSet;
        }
    }

    if (mySQLStatement != nullptr)
    {
        mySQLStatement->close();
        delete mySQLStatement;
    }
    if (mySQLResultSet != nullptr)
    {
        mySQLResultSet->close();
        delete mySQLResultSet;
    }

    if (!isWhitelistedCient)
    {
        for (size_t clientSocketsIterator = 0; clientSocketsIterator < this->clientSockets.size(); ++clientSocketsIterator)
            if (this->clientSockets[clientSocketsIterator].clientIP == clientSocket.clientIP && this->clientSockets[clientSocketsIterator].clientMAC == clientSocket.clientMAC)
            {
                while (!pthread_mutex_trylock(&this->clientSocketsMutex));
                close(clientSocket.clientSocketDescriptor);
                
                this->clientSockets[clientSocketsIterator] = this->clientSockets[this->clientSockets.size() - 1];
                this->clientSockets.pop_back();
                pthread_mutex_unlock(&this->clientSocketsMutex);

                break;
            }
        
        cout<<ERROR_CLIENT_NOT_WHITELISTED(clientSocket.clientIP)<<endl;
        return;
    }

    cout<<SUCCESS_CLIENT_CONNECTED(clientSocket.clientIP)<<endl;
}

const SpecializedServer * SpecializedServer::GetSingletonInstance()
{
    int mutexLockReturnValue = pthread_mutex_lock(&SpecializedServer::singletonInstanceMutex);
    bool isMutexInitialized = (mutexLockReturnValue != EINVAL);
    bool lockSuccess = (mutexLockReturnValue == 0);

    if (!isMutexInitialized)
        pthread_mutex_init(&SpecializedServer::singletonInstanceMutex, nullptr);

    if (!lockSuccess)
        while (!pthread_mutex_trylock(&SpecializedServer::singletonInstanceMutex));

    if (SpecializedServer::singletonInstance == nullptr)
        SpecializedServer::singletonInstance = new SpecializedServer();

    pthread_mutex_unlock(&SpecializedServer::singletonInstanceMutex);

    return SpecializedServer::singletonInstance;
}