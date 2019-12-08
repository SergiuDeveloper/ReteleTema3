#include "../Headers/LocalClient.hpp"

bool LocalClient::serverStopped = false;

SuccessState LocalClient::StopServer()
{   
    if (LocalClient::serverStopped)
        return SuccessState(false, ERROR_NO_SERVER_INSTANCE_RUNNING);

    string mySQLSuperuserPassword = getpass(PROMPT_MYSQL_DB_CREDENTIALS);

    SuccessState successState = MySQLConnector::Initialize(mySQLSuperuserPassword);
    
    if (successState.isSuccess_Get())
        cout<<successState.successStateMessage_Get()<<endl;
    else
        return successState;

    Statement * mySQLStatement;
    ResultSet * mySQLResultSet;
    string localServerPath = INVALID_SERVER_PATH;

    try
    {
        Connection * mySQLConnection = (Connection *)MySQLConnector::mySQLConnection_Get();

        mySQLStatement = mySQLConnection->createStatement();
        mySQLResultSet = mySQLStatement->executeQuery(MYSQL_GET_LOCAL_SERVER_PATH_QUERY);
    
        if (mySQLResultSet->next())
            localServerPath = mySQLResultSet->getString(1);

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

    MySQLConnector::Deinitialize();

    if (localServerPath == INVALID_SERVER_PATH)
        return SuccessState(false, ERROR_GET_LOCAL_SERVER_PATH);

    bool operationSuccess;

    int serverSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    operationSuccess = (serverSocket != -1);
    if (!operationSuccess)
        return SuccessState(false, ERROR_CLIENT_SOCKET_INITIALIZATION);

    struct sockaddr_un serverSocketAddr;
    serverSocketAddr.sun_family = AF_UNIX;
    strcpy(serverSocketAddr.sun_path, localServerPath.c_str());

    operationSuccess = (connect(serverSocket, (struct sockaddr*)&serverSocketAddr, SUN_LEN(&serverSocketAddr)) != -1);
    if (!operationSuccess)
        return SuccessState(false, ERROR_LOCAL_SERVER_CONNECTION);

    string encryptedStopServerCommandString = Encryption::SHA256::Encrypt(COMMAND_STOP_SERVER);
    char * encryptedStopServerCommand = new char[encryptedStopServerCommandString.size() + 1];
    strcpy(encryptedStopServerCommand, encryptedStopServerCommandString.c_str());
    encryptedStopServerCommand[encryptedStopServerCommandString.size()] = '\0';
    size_t encryptedStopServerCommandLength = encryptedStopServerCommandString.size();

    write(serverSocket, &encryptedStopServerCommandLength, sizeof(size_t));
    write(serverSocket, encryptedStopServerCommand, encryptedStopServerCommandLength);

    string commandSuccess = COMMAND_SUCCESS;
    string commandFailure = COMMAND_FAILURE;
    std::vector<Encryption::EncryptedValuePair> encryptedValuePairs =
    {
        Encryption::EncryptedValuePair(commandSuccess, Encryption::SHA256::Encrypt(commandSuccess)),
        Encryption::EncryptedValuePair(commandFailure, Encryption::SHA256::Encrypt(commandFailure))
    };

    struct pollfd pollDescriptor;
    pollDescriptor.fd = serverSocket;
    pollDescriptor.events = POLLIN;

    bool badAllocException = false;

    size_t commandSuccessStatusEncrpytedLength;
    char * commandSuccessStatusEncrypted;
    string commandSuccessStatus;

    int pollReturnValue = poll(&pollDescriptor, 1, DEFAULT_RECV_TIMEOUT);
    operationSuccess;
    operationSuccess = (pollReturnValue > 0);   
    if (!operationSuccess)
    {
        LocalClient::serverStopped = true;
        return SuccessState(true, SUCCESS_SERVER_STOPPED);
    }
    operationSuccess = (read(serverSocket, &commandSuccessStatusEncrpytedLength, sizeof(size_t)) > 0);
    if (!operationSuccess)
    {
        LocalClient::serverStopped = true;
        return SuccessState(true, SUCCESS_SERVER_STOPPED);
    }
    try
    {
        pollReturnValue = poll(&pollDescriptor, 1, DEFAULT_RECV_TIMEOUT);
        operationSuccess = (pollReturnValue > 0);
        if (!operationSuccess)
        {
            LocalClient::serverStopped = true;
            return SuccessState(true, SUCCESS_SERVER_STOPPED);
        }
        commandSuccessStatusEncrypted = new char[commandSuccessStatusEncrpytedLength + 1];
        operationSuccess = (read(serverSocket, commandSuccessStatusEncrypted, commandSuccessStatusEncrpytedLength) > 0);
        if (!operationSuccess)
        {
            LocalClient::serverStopped = true;
            return SuccessState(true, SUCCESS_SERVER_STOPPED);
        }
        commandSuccessStatusEncrypted[commandSuccessStatusEncrpytedLength] = '\0';
    
        commandSuccessStatus = Encryption::SHA256::Decrypt(commandSuccessStatusEncrypted, encryptedValuePairs);

        delete commandSuccessStatusEncrypted;
    }
    catch (exception & thrownException)
    {
        badAllocException = true;

        if (commandSuccessStatusEncrypted != nullptr)
            delete commandSuccessStatusEncrypted;
    }

    if (badAllocException || commandSuccessStatus == commandSuccess || commandSuccessStatus == INVALID_STRING)
    {
        LocalClient::serverStopped = true;
        return SuccessState(true, SUCCESS_SERVER_STOPPED);
    }
    
    return SuccessState(false, ERROR_SERVER_STOPPED);
}