#include "../Headers/SpecializedServer.hpp"

SpecializedServer * SpecializedServer::singletonInstance = nullptr;
pthread_mutex_t SpecializedServer::singletonInstanceMutex;

SuccessState SpecializedServer::Start(unsigned int serverPort)
{
    string mySQLSuperuserPassword = getpass(PROMPT_MYSQL_DB_CREDENTIALS);

    pthread_mutex_init(&this->consoleMutex, nullptr);
    pthread_mutex_init(&this->mySQLConnectionMutex, nullptr);

    try
    {
        this->mySQLDriver = get_driver_instance();
        this->mySQLConnection = this->mySQLDriver->connect(MYSQL_CONNECTION_STRING(MYSQL_SERVER_PROTOCOL, MYSQL_SERVER_IP, MYSQL_SERVER_PORT), MYSQL_SERVER_SUPERUSER, mySQLSuperuserPassword);
        this->mySQLConnection->setSchema(MYSQL_DATABASE_DEFAULT_SCHEMA);
    }
    catch (SQLException & mySQLException)
    {
        delete this->mySQLConnection;
        return SuccessState(false, ERROR_MYSQL_GENERIC_ERROR(mySQLException.getErrorCode(), mySQLException.what()));
    }

    cout<<SUCCESS_MYSQL_DB_CONNECTED<<endl;

    return Server::Start(serverPort);
}

SuccessState SpecializedServer::Stop()
{
    SuccessState successState = Server::Stop();
    if (!successState.isSuccess_Get())
        return successState;

    while (!pthread_mutex_trylock(&this->mySQLConnectionMutex));
    if (this->mySQLConnection != nullptr)
    {
        this->mySQLConnection->close();
        delete this->mySQLConnection;
    }
    if (this->mySQLDriver != nullptr)
        this->mySQLDriver->threadEnd();
    pthread_mutex_unlock(&this->mySQLConnectionMutex);

    pthread_mutex_destroy(&this->mySQLConnectionMutex);
    pthread_mutex_destroy(&this->consoleMutex);

    while (!pthread_mutex_trylock(&SpecializedServer::singletonInstanceMutex));
    delete SpecializedServer::singletonInstance;
    pthread_mutex_unlock(&SpecializedServer::singletonInstanceMutex);

    pthread_mutex_destroy(&SpecializedServer::singletonInstanceMutex);

    return successState;
}

void SpecializedServer::ClientConnected_EventCallback(ClientSocket clientSocket)
{
    Statement * mySQLStatement;
    ResultSet * mySQLResultSet;
    bool isWhitelistedIP = false;

    try
    {
        mySQLStatement = this->mySQLConnection->createStatement();
        mySQLResultSet = mySQLStatement->executeQuery(MYSQL_GET_WHITELISTED_IP_COUNT_QUERY(clientSocket.clientIP));

        if (mySQLResultSet->next())
            isWhitelistedIP = (mySQLResultSet->getInt(1) > 0);
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

    if (!isWhitelistedIP)
    {
        for (size_t clientSocketsIterator = 0; clientSocketsIterator < this->clientSockets.size(); ++clientSocketsIterator)
            if (this->clientSockets[clientSocketsIterator].clientIP == clientSocket.clientIP)
            {
                while (!pthread_mutex_trylock(&this->clientSocketsMutex));
                close(clientSocket.clientSocketDescriptor);
                
                this->clientSockets[clientSocketsIterator] = this->clientSockets[this->clientSockets.size() - 1];
                this->clientSockets.pop_back();
                pthread_mutex_unlock(&this->clientSocketsMutex);
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