#include "../Headers/LocalServer.hpp"

LocalServer * LocalServer::singletonInstance;
pthread_mutex_t LocalServer::singletonInstanceMutex;

SuccessState LocalServer::Start()
{
    if (this->serverRunning_Get())
        return SuccessState(false, ERROR_SERVER_ALREADY_RUNNING);

    PreparedStatement * mySQLStatement;

    try
    {
        Connection * mySQLConnection = MySQLConnector::mySQLConnection_Get();

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
    size_t encryptedCommandLength;

    read(clientSocket.clientSocketDescriptor, &encryptedCommandLength, sizeof(size_t));
    char * encrpytedCommand = new char[encryptedCommandLength];
    read(clientSocket.clientSocketDescriptor, encrpytedCommand, encryptedCommandLength);

    string commandStopServer = COMMAND_STOP_SERVER;
    std::vector<Encryption::EncryptedValuePair> encryptedValuePairs =
    {
        Encryption::EncryptedValuePair(commandStopServer, Encryption::SHA256::Encrypt(commandStopServer))
    };

    string receivedCommand = Encryption::SHA256::Decrypt(encrpytedCommand, encryptedValuePairs);

    delete encrpytedCommand;

    bool successfullyStoppedServer = false;
    if (receivedCommand == commandStopServer)
    {
        SpecializedServer * specializedServer = (SpecializedServer *)SpecializedServer::GetSingletonInstance();

        if (specializedServer != nullptr && specializedServer->serverRunning_Get())
            successfullyStoppedServer = specializedServer->Stop().isSuccess_Get();
    }

    string commandToSend = (successfullyStoppedServer ? COMMAND_SUCCESS : COMMAND_FAILURE); 
    string encryptedCommandToSend = Encryption::SHA256::Encrypt(commandToSend);
    size_t encrpytedCommandToSendLength = encryptedCommandToSend.size();

    write(clientSocket.clientSocketDescriptor, &encrpytedCommandToSendLength, sizeof(size_t));
    write(clientSocket.clientSocketDescriptor, encryptedCommandToSend.c_str(), encryptedCommandLength);

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