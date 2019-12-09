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
            for (auto & dbClientMACIterator : dbClientMAC)
                dbClientMACIterator = toupper(dbClientMACIterator);
                
            dbClientMACEncrypted = Encryption::Algorithms::SHA256::Encrypt(dbClientMAC);

            if (dbClientMACEncrypted == clientSocket.clientMAC)
            {
                decryptedClientMAC =    dbClientMAC[0] + dbClientMAC[1] + ':' +
                                        dbClientMAC[2] + dbClientMAC[3] + ':' +
                                        dbClientMAC[4] + dbClientMAC[5] + ':' +
                                        dbClientMAC[6] + dbClientMAC[7] + ':' +
                                        dbClientMAC[8] + dbClientMAC[9] + ':' +
                                        dbClientMAC[10] + dbClientMAC[11];
                clientSocket.clientMAC = decryptedClientMAC;

                isWhitelistedMAC = true;
                break;
            }
        }
    }

    bool operationSuccess;

    char * clientUsername;
    size_t clientUsernameLength;
    if (isWhitelistedIP && isWhitelistedMAC)
    {
        operationSuccess = (read(clientSocket.clientSocketDescriptor, &clientUsernameLength, sizeof(size_t)) > 0);
        if (operationSuccess)
        {
            clientUsername = new char[clientUsernameLength + 1];

            operationSuccess = (read(clientSocket.clientSocketDescriptor, clientUsername, clientUsernameLength) > 0);
            clientUsername[clientUsernameLength] = '\0';
            if (!operationSuccess)
            {
                isWhitelistedIP = false;
                isWhitelistedMAC = false;

                delete clientUsername;
            }
        }
        else
        {
            isWhitelistedIP = false;
            isWhitelistedMAC = false;
        }
    }

    char * clientPassword;
    size_t clientPasswordLength;
    if (isWhitelistedIP && isWhitelistedMAC)
    {
        operationSuccess = (read(clientSocket.clientSocketDescriptor, &clientPasswordLength, sizeof(size_t)) > 0);
        if (operationSuccess)
        {
            clientPassword = new char[clientPasswordLength + 1];

            operationSuccess = (read(clientSocket.clientSocketDescriptor, clientPassword, clientPasswordLength) > 0);
            clientPassword[clientPasswordLength] = '\0';
            if (!operationSuccess)
            {
                isWhitelistedIP = false;
                isWhitelistedMAC = false;

                delete clientUsername;
                delete clientPassword;
            }
        }
        else
        {
            isWhitelistedIP = false;
            isWhitelistedMAC = false;

            delete clientUsername;
        }
        
    }

    bool isAdmin = false;

    if (isWhitelistedIP && isWhitelistedMAC)
    {
        vector<pair<string, string>> administratorCredentials;

        Statement * mySQLNormalStatement;

        try
        {
            Connection * mySQLConnection = (Connection *)MySQLConnector::mySQLConnection_Get();
            
            mySQLNormalStatement = mySQLConnection->createStatement();
            mySQLResultSet = mySQLNormalStatement->executeQuery(MYSQL_GET_ALL_ADMINISTRATOR_CREDENTIALS);
            
            string adminUsername, adminPassword;
            pair<string, string> administratorCredential;

            while (mySQLResultSet->next())
            {
                adminUsername = mySQLResultSet->getString(1);
                adminPassword = mySQLResultSet->getString(2);

                administratorCredential = pair<string, string>(adminUsername, adminPassword);

                administratorCredentials.push_back(administratorCredential);
            }

            while (mySQLNormalStatement->getMoreResults())
                mySQLResultSet = mySQLNormalStatement->getResultSet();
        }
        catch (SQLException & mySQLException)
        {
            cout<<ERROR_MYSQL_GENERIC_ERROR(mySQLException.getErrorCode(), mySQLException.what())<<endl;

            while (mySQLResultSet->next());
            while (mySQLNormalStatement->getMoreResults())
                mySQLResultSet = mySQLNormalStatement->getResultSet();

            if (mySQLNormalStatement != nullptr)
            {
                mySQLNormalStatement->close();
                delete mySQLNormalStatement;
            }
            if (mySQLResultSet != nullptr)
            {
                mySQLResultSet->close();
                delete mySQLResultSet;
            }
        }

        if (mySQLNormalStatement != nullptr)
        {
            mySQLNormalStatement->close();
            delete mySQLNormalStatement;
        }
        if (mySQLResultSet != nullptr)
        {
            mySQLResultSet->close();
            delete mySQLResultSet;
        }

        string clientUsernameString = clientUsername;
        string clientPasswordString = clientPassword;

        for (auto & administratorCredential : administratorCredentials)
        {
            administratorCredential.first = Encryption::Algorithms::SHA256::Encrypt(administratorCredential.first);
            if (administratorCredential.first == clientUsernameString && administratorCredential.second == clientPasswordString)
                isAdmin = true;
        }
    }

    bool isWhitelistedClient = isWhitelistedIP && isWhitelistedMAC && isAdmin;

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

                    cout<<ERROR_CLIENT_NOT_WHITELISTED(clientSocket.clientIP)<<endl;
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

    string successMessage = MESSAGE_SUCCESS;
    size_t successMessageLength = successMessage.size();

    write(clientSocket.clientSocketDescriptor, &successMessageLength, sizeof(size_t));
    write(clientSocket.clientSocketDescriptor, successMessage.c_str(), successMessageLength);

    cout<<SUCCESS_CLIENT_CONNECTED(clientSocket.clientIP, clientSocket.clientMAC)<<endl;

    int readResult;
    bool clientConnected = true;
    Encryption::Types::CharArray encryptedClientRequest;
    string commandExecutionPath = DEFAULT_EXECUTION_PATH;
    while (clientConnected)
    {    
        readResult = (read(clientSocket.clientSocketDescriptor, &encryptedClientRequest.charArrayLength, sizeof(size_t)));

        if (readResult == 0)
        {
            this->ClientDisconnected_EventCallback(clientSocket);
            return;
        }
        else if (readResult < 0)
            continue;

        encryptedClientRequest.charArray = new char[encryptedClientRequest.charArrayLength];
        readResult = (read(clientSocket.clientSocketDescriptor, encryptedClientRequest.charArray, encryptedClientRequest.charArrayLength));

        if (readResult == 0)
            this->ClientDisconnected_EventCallback(clientSocket);
        else if (readResult > 0)
            this->ClientRequest_EventCallback(clientSocket, encryptedClientRequest, commandExecutionPath);
    }

    this->ClientDisconnected_EventCallback(clientSocket);
}

void SpecializedServer::ClientRequest_EventCallback(ClientSocket clientSocket, Encryption::Types::CharArray messageToProcess, string & commandExecutionPath)
{
    string clientRequest = Encryption::Algorithms::Vigenere::Decrypt(messageToProcess, VIGENERE_KEY(this->serverPort_Get(), clientSocket.clientMAC), VIGENERE_RANDOM_PREFIX_LENGTH, VIGENERE_RANDOM_SUFFIX_LENGTH);

    cout<<SUCCESS_RECEIVED_REQUEST(clientSocket.clientIP, clientSocket.clientMAC, clientRequest)<<endl;

    clientRequest = "cd " + commandExecutionPath + " && echo \"$(whoami)@$(hostname)\" && pwd && ";

    bool operationSuccess;

    FILE * commandLinePipe;
    operationSuccess = ((commandLinePipe = popen(clientRequest.c_str(), "r")) != nullptr);

    if (!operationSuccess)
    {
        string messageFailure = MESSAGE_FAILURE;
        Encryption::Types::CharArray messageFailureEncrypted = Encryption::Algorithms::Vigenere::Encrypt(messageFailure, VIGENERE_KEY(this->serverPort_Get(), clientSocket.clientMAC), VIGENERE_RANDOM_PREFIX_LENGTH,
            VIGENERE_RANDOM_SUFFIX_LENGTH);

        write(clientSocket.clientSocketDescriptor, &messageFailureEncrypted.charArrayLength, sizeof(size_t));
        write(clientSocket.clientSocketDescriptor, messageFailureEncrypted.charArray, messageFailureEncrypted.charArrayLength);

        cout<<ERROR_EXECUTE_COMMAND(clientSocket.clientIP, clientSocket.clientMAC)<<endl;
        return;
    }

    bool currentDirectoryLog = true;

    string commandOutput = "";
    size_t outputCharsToRead = SOCKET_CHUNK_SIZE - VIGENERE_RANDOM_PREFIX_LENGTH - VIGENERE_RANDOM_SUFFIX_LENGTH;
    char * commandOutputLine = new char[outputCharsToRead];
    while (fgets(commandOutputLine, outputCharsToRead, commandLinePipe))
    {
        commandOutput += (string)commandOutputLine;

        if (currentDirectoryLog)
        {
            commandExecutionPath = (string)commandOutputLine;

            currentDirectoryLog = false;
        }
    }
    
    pclose(commandLinePipe);

    commandOutput.resize(outputCharsToRead);

    Encryption::Types::CharArray commandOutputEncrypted = Encryption::Algorithms::Vigenere::Encrypt(commandOutput, VIGENERE_KEY(this->serverPort_Get(), clientSocket.clientMAC), VIGENERE_RANDOM_PREFIX_LENGTH,
        VIGENERE_RANDOM_SUFFIX_LENGTH);

    write(clientSocket.clientSocketDescriptor, &commandOutputEncrypted.charArrayLength, sizeof(size_t));
    write(clientSocket.clientSocketDescriptor, commandOutputEncrypted.charArray, commandOutputEncrypted.charArrayLength);

    delete commandOutputLine;

    cout<<SUCCESS_EXECUTED_COMMAND(clientSocket.clientIP, clientSocket.clientMAC)<<endl;
}

void SpecializedServer::ClientDisconnected_EventCallback(ClientSocket clientSocket)
{
    for (size_t clientSocketsIterator = 0; clientSocketsIterator < this->clientSockets.size(); ++clientSocketsIterator)
        if (this->clientSockets[clientSocketsIterator].clientIP == clientSocket.clientIP && this->clientSockets[clientSocketsIterator].clientMAC == clientSocket.clientMAC)
        {
            while (!pthread_mutex_trylock(&this->clientSocketsMutex));
            close(clientSocket.clientSocketDescriptor);
                    
            this->clientSockets[clientSocketsIterator] = this->clientSockets[this->clientSockets.size() - 1];
            this->clientSockets.pop_back();
            pthread_mutex_unlock(&this->clientSocketsMutex);

            cout<<SUCCESS_CLIENT_DISCONNECTED(clientSocket.clientIP, clientSocket.clientMAC)<<endl;
            return;
        }
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