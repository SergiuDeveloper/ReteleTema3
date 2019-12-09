#include "../Headers/LocalServer.hpp"

LocalServer * LocalServer::singletonInstance;
pthread_mutex_t LocalServer::singletonInstanceMutex;

LocalServer::LocalServer()
{
}

SuccessState LocalServer::Start()
{
    if (this->serverRunning_Get())
        return SuccessState(false, ERROR_SERVER_ALREADY_RUNNING);

    PreparedStatement * mySQLStatement;

    try
    {
        Connection * mySQLConnection = (Connection *)MySQLConnector::mySQLConnection_Get();

        mySQLStatement = mySQLConnection->prepareStatement(MYSQL_UPDATE_LOCAL_SERVER_PATH_QUERY);
        mySQLStatement->setString(1, LOCAL_SERVER_PATH);
        mySQLStatement->execute();
    }
    catch (SQLException & mySQLException)
    {
        cout<<ERROR_MYSQL_GENERIC_ERROR(mySQLException.getErrorCode(), mySQLException.what())<<endl;

        if (mySQLStatement != nullptr)
        {
            mySQLStatement->close();
            delete mySQLStatement;
        }

        return SuccessState(false, ERROR_MYSQL_LOG_LOCAL_SERVER_PATH);
    }

    if (mySQLStatement != nullptr)
    {
        mySQLStatement->close();
        delete mySQLStatement;
    }

    return Server::Start(LOCAL_SERVER_PATH);
}

SuccessState LocalServer::Stop()
{
    pthread_mutex_destroy(&LocalServer::singletonInstanceMutex);

    return Server::Stop();
}

void LocalServer::ClientConnected_EventCallback(ClientSocket clientSocket)
{
    size_t encryptedCommandLength;
    read(clientSocket.clientSocketDescriptor, &encryptedCommandLength, sizeof(size_t));
    char * encryptedCommand = new char[encryptedCommandLength + 1];
    read(clientSocket.clientSocketDescriptor, encryptedCommand, encryptedCommandLength);
    encryptedCommand[encryptedCommandLength] = '\0';
    
    string commandStopServer = COMMAND_STOP_SERVER;
    vector<Encryption::Types::EncryptedValuePair> encryptedValuePairs =
    {
        Encryption::Types::EncryptedValuePair(commandStopServer, Encryption::Algorithms::SHA256::Encrypt(commandStopServer))
    };
    
    string receivedCommand = Encryption::Algorithms::SHA256::Decrypt(encryptedCommand, encryptedValuePairs);
    delete encryptedCommand;
    
    bool successfullyStoppedServer = false;
    if (receivedCommand == commandStopServer)
    {
        SpecializedServer * specializedServer = (SpecializedServer *)SpecializedServer::GetSingletonInstance();
        
        if (specializedServer != nullptr && specializedServer->serverRunning_Get())
            successfullyStoppedServer = specializedServer->Stop().isSuccess_Get();
    }
    
    string commandToSend = (successfullyStoppedServer ? COMMAND_SUCCESS : COMMAND_FAILURE); 
    string encryptedCommandToSendString = Encryption::Algorithms::SHA256::Encrypt(commandToSend);
    char * encryptedCommandToSend = new char[encryptedCommandToSendString.size() + 1];
    strcpy(encryptedCommandToSend, encryptedCommandToSendString.c_str());
    encryptedCommandToSend[encryptedCommandToSendString.size()] = '\0';
    size_t encrpytedCommandToSendLength = encryptedCommandToSendString.size();
    
    write(clientSocket.clientSocketDescriptor, &encrpytedCommandToSendLength, sizeof(size_t));
    write(clientSocket.clientSocketDescriptor, encryptedCommandToSend, encryptedCommandLength);
    
    if (successfullyStoppedServer)
        this->Stop();
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