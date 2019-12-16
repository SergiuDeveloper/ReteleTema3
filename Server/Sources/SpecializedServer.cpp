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

    if (RDCStreamingServer::IsGraphicsCompatible())
    {
        bool rdcStreamingServerStarted = RDCStreamingServer::Start();

        while (!pthread_mutex_trylock(&this->consoleMutex));
        cout<<(rdcStreamingServerStarted ? SUCCESS_RDC_STREAMING_SERVER_STARTED : FAILURE_RDC_STREAMING_SERVER_STARTED)<<endl;
        pthread_mutex_unlock(&this->consoleMutex);
    }
    
    return successState;
}

SuccessState SpecializedServer::Stop()
{
    MySQLConnector::Deinitialize();
    
    pthread_mutex_destroy(&this->consoleMutex);
    pthread_mutex_destroy(&SpecializedServer::singletonInstanceMutex);

    if (RDCStreamingServer::IsGraphicsCompatible() && RDCStreamingServer::isRunning_Get())
    {
        bool rdcStreamingServerStopped = RDCStreamingServer::Stop();
        cout<<(rdcStreamingServerStopped ? SUCCESS_RDC_STREAMING_SERVER_STOPPED : FAILURE_RDC_STREAMING_SERVER_STOPPED)<<endl;
    }
    
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
        while (!pthread_mutex_trylock(&this->consoleMutex));
        cout<<ERROR_MYSQL_GENERIC_ERROR(mySQLException.getErrorCode(), mySQLException.what())<<endl;
        pthread_mutex_unlock(&this->consoleMutex);

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
            while (!pthread_mutex_trylock(&this->consoleMutex));
            cout<<ERROR_MYSQL_GENERIC_ERROR(mySQLException.getErrorCode(), mySQLException.what())<<endl;
            pthread_mutex_unlock(&this->consoleMutex);

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
                char * decryptedClientMAC = new char[dbClientMAC.size() + 6];
                sprintf(decryptedClientMAC, "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c", dbClientMAC[0], dbClientMAC[1], dbClientMAC[2], dbClientMAC[3], dbClientMAC[4], dbClientMAC[5], dbClientMAC[6], dbClientMAC[7], dbClientMAC[8],
                    dbClientMAC[9], dbClientMAC[10], dbClientMAC[11]);

                for (auto & clientSocketsIterator : this->clientSockets)
                    if (clientSocketsIterator.clientIP == clientSocket.clientIP && clientSocketsIterator.clientMAC == clientSocket.clientMAC)
                        clientSocketsIterator.clientMAC = decryptedClientMAC;
                clientSocket.clientMAC = decryptedClientMAC;

                delete decryptedClientMAC;

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
            while (!pthread_mutex_trylock(&this->consoleMutex));
            cout<<ERROR_MYSQL_GENERIC_ERROR(mySQLException.getErrorCode(), mySQLException.what())<<endl;
            pthread_mutex_unlock(&this->consoleMutex);

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
                this->clientSockets[clientSocketsIterator].clientMAC = clientSocket.clientMAC;
            else
            {
                close(clientSocket.clientSocketDescriptor);
                
                this->clientSockets[clientSocketsIterator] = this->clientSockets[this->clientSockets.size() - 1];
                this->clientSockets.pop_back();

                pthread_mutex_unlock(&this->clientSocketsMutex);

                while (!pthread_mutex_trylock(&this->consoleMutex));
                cout<<ERROR_CLIENT_NOT_WHITELISTED(clientSocket.clientIP)<<endl;
                pthread_mutex_unlock(&this->consoleMutex);

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

                while (!pthread_mutex_trylock(&this->consoleMutex));
                cout<<ERROR_CLIENT_ALREADY_CONNECTED(clientSocket.clientIP, clientSocket.clientMAC)<<endl;
                pthread_mutex_unlock(&this->consoleMutex);

                return;
            }
            else
                foundClient = true;

    string successMessage = MESSAGE_SUCCESS;
    string successMessageEncrypted = Encryption::Algorithms::SHA256::Encrypt(successMessage);
    size_t successMessageEncryptedLength = successMessageEncrypted.size();

    write(clientSocket.clientSocketDescriptor, &successMessageEncryptedLength, sizeof(size_t));
    write(clientSocket.clientSocketDescriptor, successMessageEncrypted.c_str(), successMessageEncryptedLength);

    while (!pthread_mutex_trylock(&this->consoleMutex));
    cout<<SUCCESS_CLIENT_CONNECTED(clientSocket.clientIP, clientSocket.clientMAC)<<endl;
    pthread_mutex_unlock(&this->consoleMutex);

    char hostName[HOST_NAME_MAX + 1];
    gethostname(hostName, HOST_NAME_MAX);
    char * userName = getenv(UNIX_USERNAME_ENV);

    string clientRequest = (string)"pwd && echo " + userName + "@" + hostName + " && exit";

    string commandResponse;
    string commandExecutionPath = DEFAULT_EXECUTION_PATH;

    int pipeDescriptor[2];
    pipe(pipeDescriptor);

    int forkReturnValue = fork();

    switch (forkReturnValue)
    {
        case -1:
        {
            string messageFailure = MESSAGE_FAILURE;
            Encryption::Types::CharArray messageFailureEncrypted = Encryption::Algorithms::Vigenere::Encrypt(messageFailure, VIGENERE_KEY(this->serverPort_Get(), clientSocket.clientMAC), VIGENERE_RANDOM_PREFIX_LENGTH,
                VIGENERE_RANDOM_SUFFIX_LENGTH);

            write(clientSocket.clientSocketDescriptor, &messageFailureEncrypted.charArrayLength, sizeof(size_t));
            write(clientSocket.clientSocketDescriptor, messageFailureEncrypted.charArray, messageFailureEncrypted.charArrayLength);

            break;
        }
        case 0:
        {
            dup2(pipeDescriptor[1], STDOUT_FILENO);
            close(pipeDescriptor[0]);
            close(pipeDescriptor[1]);

            const char * commandPath = "/bin/sh";
            const char * commandArguments[] = { commandPath, "-c", clientRequest.c_str(), nullptr };

            execvp(commandPath, (char **)commandArguments);
 
            exit(EXIT_FAILURE);

            break;
        }
        default:
        {
            close(pipeDescriptor[1]);

            commandResponse = "";

            size_t commandOutputMaximumLegth = SOCKET_CHUNK_SIZE - VIGENERE_RANDOM_PREFIX_LENGTH - VIGENERE_RANDOM_SUFFIX_LENGTH;
            char * commandOutput = new char[commandOutputMaximumLegth];
            int commandOutputLength;
            while ((commandOutputLength = read(pipeDescriptor[0], commandOutput, commandOutputMaximumLegth)) > 0)
            {
                commandOutput[commandOutputLength] = '\0';
                commandResponse += commandOutput;
            }

            size_t newlinePosition = commandResponse.find('\n');
            if (newlinePosition == string::npos)
            {
                commandExecutionPath = commandResponse;
                commandResponse.clear();
            }
            else
                commandExecutionPath = commandResponse.substr(0, newlinePosition);
            
            if (commandResponse.size() > commandOutputMaximumLegth)
                commandResponse.resize(commandOutputMaximumLegth);

            close(pipeDescriptor[0]);

            delete commandOutput;

            break;
        }
    }

    Encryption::Types::CharArray commandOutputEncrypted = Encryption::Algorithms::Vigenere::Encrypt(commandResponse, VIGENERE_KEY(this->serverPort_Get(), clientSocket.clientMAC), VIGENERE_RANDOM_PREFIX_LENGTH,
        VIGENERE_RANDOM_SUFFIX_LENGTH);

    write(clientSocket.clientSocketDescriptor, &commandOutputEncrypted.charArrayLength, sizeof(size_t));
    write(clientSocket.clientSocketDescriptor, commandOutputEncrypted.charArray, commandOutputEncrypted.charArrayLength);

    int readResult;
    bool clientConnected = true;
    Encryption::Types::CharArray encryptedClientRequest;
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

    while (!pthread_mutex_trylock(&this->consoleMutex));
    cout<<SUCCESS_RECEIVED_REQUEST(clientSocket.clientIP, clientSocket.clientMAC, clientRequest)<<endl;
    pthread_mutex_unlock(&this->consoleMutex);

    string messageConnectRDC = MESSAGE_CONNECT_RDC;
    if (clientRequest.size() == messageConnectRDC.size())
    {
        string requestUpper = "";
        for (auto & clientRequestChar : clientRequest)
            if (clientRequestChar >= 'a' && clientRequestChar <= 'z')
                requestUpper += (clientRequestChar + 'A' - 'a');
            else
                requestUpper += clientRequestChar;

        if (requestUpper == messageConnectRDC)
        {
            unsigned int rdcStreamingServerPort = RDCStreamingServer::serverPort_Get();
            string rdcStreamingServerPortString = to_string(rdcStreamingServerPort);
            Encryption::Types::CharArray rdcStreamingServerPortStringEncrypted = Encryption::Algorithms::Vigenere::Encrypt(rdcStreamingServerPortString, VIGENERE_KEY(this->serverPort_Get(), clientSocket.clientMAC),
                VIGENERE_RANDOM_PREFIX_LENGTH, VIGENERE_RANDOM_SUFFIX_LENGTH);

            write(clientSocket.clientSocketDescriptor, &rdcStreamingServerPortStringEncrypted.charArrayLength, sizeof(size_t));
            write(clientSocket.clientSocketDescriptor, rdcStreamingServerPortStringEncrypted.charArray, rdcStreamingServerPortStringEncrypted.charArrayLength);

            bool addedWhitelistedClientToRDCStreaming = RDCStreamingServer::AddWhitelistedClient(clientSocket.clientSocketAddr);
            cout<<(addedWhitelistedClientToRDCStreaming ? SUCCESS_ADDED_CLIENT_TO_RDC_STREAMING : FAILURE_ADDED_CLIENT_TO_RDC_STREAMING)<<endl;

            return;
        }            
    }

    char hostName[HOST_NAME_MAX + 1];
    gethostname(hostName, HOST_NAME_MAX);
    char * userName = getenv(UNIX_USERNAME_ENV);

    clientRequest = "cd \"" + commandExecutionPath + "\" && echo " + userName + "@" + hostName + " && " + clientRequest + " && pwd && exit";

    string commandResponse;

    int pipeDescriptor[2];
    pipe(pipeDescriptor);

    int forkReturnValue = fork();

    switch (forkReturnValue)
    {
        case -1:
        {
            string messageFailure = MESSAGE_FAILURE;
            Encryption::Types::CharArray messageFailureEncrypted = Encryption::Algorithms::Vigenere::Encrypt(messageFailure, VIGENERE_KEY(this->serverPort_Get(), clientSocket.clientMAC), VIGENERE_RANDOM_PREFIX_LENGTH,
                VIGENERE_RANDOM_SUFFIX_LENGTH);

            write(clientSocket.clientSocketDescriptor, &messageFailureEncrypted.charArrayLength, sizeof(size_t));
            write(clientSocket.clientSocketDescriptor, messageFailureEncrypted.charArray, messageFailureEncrypted.charArrayLength);

            while (!pthread_mutex_trylock(&this->consoleMutex));
            cout<<ERROR_EXECUTE_COMMAND(clientSocket.clientIP, clientSocket.clientMAC)<<endl;
            pthread_mutex_unlock(&this->consoleMutex);
            
            break;
        }
        case 0:
        {
            dup2(pipeDescriptor[1], STDOUT_FILENO);
            close(pipeDescriptor[0]);
            close(pipeDescriptor[1]);

            const char * commandPath = "/bin/sh";
            const char * commandArguments[] = { commandPath, "-c", clientRequest.c_str(), nullptr };

            execvp(commandPath, (char **)commandArguments);
 
            exit(EXIT_FAILURE);

            break;
        }
        default:
        {
            close(pipeDescriptor[1]);

            commandResponse = "";

            size_t commandOutputMaximumLegth = SOCKET_CHUNK_SIZE - VIGENERE_RANDOM_PREFIX_LENGTH - VIGENERE_RANDOM_SUFFIX_LENGTH;
            char * commandOutput = new char[commandOutputMaximumLegth + 1];
            int commandOutputLength;
            while ((commandOutputLength = read(pipeDescriptor[0], commandOutput, commandOutputMaximumLegth)) > 0)
            {
                commandOutput[commandOutputLength] = '\0';
                commandResponse += commandOutput;
            }

            close(pipeDescriptor[0]);

            commandExecutionPath = commandResponse;
            while (commandExecutionPath[commandExecutionPath.size() - 1] == '\n')
                commandExecutionPath.pop_back();
            commandExecutionPath = commandExecutionPath.substr(commandExecutionPath.rfind('\n') + 1);

            if (commandResponse.size() > commandOutputMaximumLegth)
                commandResponse.resize(commandOutputMaximumLegth);

            delete commandOutput;

            break;
        }
    }

    Encryption::Types::CharArray commandOutputEncrypted = Encryption::Algorithms::Vigenere::Encrypt(commandResponse, VIGENERE_KEY(this->serverPort_Get(), clientSocket.clientMAC), VIGENERE_RANDOM_PREFIX_LENGTH,
        VIGENERE_RANDOM_SUFFIX_LENGTH);

    write(clientSocket.clientSocketDescriptor, &commandOutputEncrypted.charArrayLength, sizeof(size_t));
    write(clientSocket.clientSocketDescriptor, commandOutputEncrypted.charArray, commandOutputEncrypted.charArrayLength);

    while (!pthread_mutex_trylock(&this->consoleMutex));
    cout<<SUCCESS_EXECUTED_COMMAND(clientSocket.clientIP, clientSocket.clientMAC)<<endl;
    pthread_mutex_unlock(&this->consoleMutex);
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

            while (!pthread_mutex_trylock(&this->consoleMutex));
            cout<<SUCCESS_CLIENT_DISCONNECTED(clientSocket.clientIP, clientSocket.clientMAC)<<endl;
            pthread_mutex_unlock(&this->consoleMutex);
            
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