#include "../Headers/SpecializedServer.hpp"

SpecializedServer * SpecializedServer::singletonInstance = nullptr;
pthread_mutex_t SpecializedServer::singletonInstanceMutex;

SpecializedServer::SpecializedServer()
{
}

SuccessState SpecializedServer::Start(unsigned int serverPort)
{
    if (this->serverRunning_Get())
        return SuccessState(false, ERROR_SERVER_ALREADY_RUNNING);

    string mySQLSuperuserPassword = getpass(PROMPT_MYSQL_DB_CREDENTIALS);

    SuccessState successState = MySQLConnector::Initialize(mySQLSuperuserPassword);
    
    if (successState.isSuccess_Get())
        cout<<successState.successStateMessage_Get()<<endl;
    else
        return successState;
    
    pthread_mutex_init(&this->consoleMutex, nullptr);
    
    successState = Server::Start(serverPort);

    if (!successState.isSuccess_Get())
        return successState;

    this->localServer = (LocalServer *)LocalServer::GetSingletonInstance();
    SuccessState localServerSuccessState = this->localServer->Start();
    if (!localServerSuccessState.isSuccess_Get())
    {
        Server::Stop();
        
        return localServerSuccessState;
    }

    SpecializedServer::singletonInstance = this;
    
    return successState;
}

SuccessState SpecializedServer::Stop()
{
    MySQLConnector::Deinitialize();
    
    pthread_mutex_destroy(&this->consoleMutex);
    pthread_mutex_destroy(&SpecializedServer::singletonInstanceMutex);
    
    return Server::Stop();
}

void SpecializedServer::ClientConnected_EventCallback(ClientSocket clientSocket)
{
    PreparedStatement * mySQLStatement;
    ResultSet * mySQLResultSet;
    bool isWhitelistedIP = false;

    try
    {
        Connection * mySQLConnection = (Connection *)MySQLConnector::mySQLConnection_Get();
        
        mySQLStatement = mySQLConnection->prepareStatement(MYSQL_IS_WHITELISTED_IP_QUERY);
        mySQLStatement->setString(1, clientSocket.clientIP);
        mySQLResultSet = mySQLStatement->executeQuery();
        
        if (mySQLResultSet->next())
            isWhitelistedIP = mySQLResultSet->getBoolean(1);

        while (mySQLResultSet->next());
        while (mySQLStatement->getMoreResults())
            mySQLResultSet = mySQLStatement->getResultSet();
    }
    catch (SQLException & mySQLException)
    {
        cout<<ERROR_MYSQL_GENERIC_ERROR(mySQLException.getErrorCode(), mySQLException.what())<<endl;

        while (mySQLResultSet->next());
        while (mySQLStatement->getMoreResults())
            mySQLResultSet = mySQLStatement->getResultSet();

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

    string decryptedClientMAC;

    bool isWhitelistedMAC = false;
    if (isWhitelistedIP)
    {
        vector<string> dbClientMACs;

        try
        {
            Connection * mySQLConnection = (Connection *)MySQLConnector::mySQLConnection_Get();
            
            mySQLStatement = mySQLConnection->prepareStatement(MYSQL_GET_MACS_FOR_WHITELISTED_IP_QUERY);
            mySQLStatement->setString(1, clientSocket.clientIP);
            mySQLResultSet = mySQLStatement->executeQuery();
            
            string dbClientMAC;

            while (mySQLResultSet->next())
            {
                dbClientMAC = mySQLResultSet->getString(1);

                dbClientMACs.push_back(dbClientMAC);
            }

            while (mySQLStatement->getMoreResults())
                mySQLResultSet = mySQLStatement->getResultSet();
        }
        catch (SQLException & mySQLException)
        {
            cout<<ERROR_MYSQL_GENERIC_ERROR(mySQLException.getErrorCode(), mySQLException.what())<<endl;

            while (mySQLResultSet->next());
            while (mySQLStatement->getMoreResults())
                mySQLResultSet = mySQLStatement->getResultSet();

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

        string dbClientMACEncrypted;
        for (auto & dbClientMAC : dbClientMACs)
        {
            dbClientMACEncrypted = Encryption::SHA256::Encrypt(dbClientMAC);

            if (dbClientMACEncrypted == clientSocket.clientMAC)
            {
                isWhitelistedMAC = true;
                break;
            }
        }
    }

    bool isWhitelistedClient = isWhitelistedIP && isWhitelistedMAC;

    for (size_t clientSocketsIterator = 0; clientSocketsIterator < this->clientSockets.size(); ++clientSocketsIterator)
            if (this->clientSockets[clientSocketsIterator].clientIP == clientSocket.clientIP && this->clientSockets[clientSocketsIterator].clientMAC == clientSocket.clientMAC)
            {
                while (!pthread_mutex_trylock(&this->clientSocketsMutex));

                if (isWhitelistedClient)
                    this->clientSockets[clientSocketsIterator].clientMAC = decryptedClientMAC;
                else
                {
                    close(clientSocket.clientSocketDescriptor);
                
                    this->clientSockets[clientSocketsIterator] = this->clientSockets[this->clientSockets.size() - 1];
                    this->clientSockets.pop_back();

                    pthread_mutex_unlock(&this->clientSocketsMutex);

                    cout<<ERROR_CLIENT_NOT_WHITELISTED(clientSocket.clientIP, clientSocket.clientMAC)<<endl;
                    return;
                }

                pthread_mutex_unlock(&this->clientSocketsMutex);
            }
    
    bool foundClient = false;
    for (size_t clientSocketsIterator = 0; clientSocketsIterator < this->clientSockets.size(); ++clientSocketsIterator)
        if (this->clientSockets[clientSocketsIterator].clientIP == clientSocket.clientIP && this->clientSockets[clientSocketsIterator].clientMAC == clientSocket.clientMAC)
            if (foundClient)
            {
                while (!pthread_mutex_trylock(&this->clientSocketsMutex));
                close(clientSocket.clientSocketDescriptor);
                    
                this->clientSockets[clientSocketsIterator] = this->clientSockets[this->clientSockets.size() - 1];
                this->clientSockets.pop_back();
                pthread_mutex_unlock(&this->clientSocketsMutex);

                cout<<ERROR_CLIENT_ALREADY_CONNECTED(clientSocket.clientIP, clientSocket.clientMAC)<<endl;
                return;
            }
            else
                foundClient = true;

    cout<<SUCCESS_CLIENT_CONNECTED(clientSocket.clientIP, clientSocket.clientMAC)<<endl;
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