#include "../Headers/SpecializedServer.hpp"

pthread_mutex_t SpecializedServer::consoleMutex;
Connection * SpecializedServer::mySQLConnection;
pthread_mutex_t SpecializedServer::mySQLConnectionMutex;

SuccessState SpecializedServer::Start(unsigned int serverPort)
{
    string mySQLSuperuserPassword = getpass(PROMPT_MYSQL_DB_CREDENTIALS);

    pthread_mutex_init(&SpecializedServer::consoleMutex, nullptr);
    pthread_mutex_init(&SpecializedServer::mySQLConnectionMutex, nullptr);

    try
    {
        Driver * mySQLDriver = get_driver_instance();
        SpecializedServer::mySQLConnection = mySQLDriver->connect(MYSQL_CONNECTION_STRING(MYSQL_SERVER_PROTOCOL, MYSQL_SERVER_IP, MYSQL_SERVER_PORT), MYSQL_SERVER_SUPERUSER, mySQLSuperuserPassword);
        SpecializedServer::mySQLConnection->setSchema(MYSQL_DATABASE_DEFAULT_SCHEMA);
    }
    catch (SQLException & mySQLException)
    {
        delete SpecializedServer::mySQLConnection;
        return SuccessState(false, ERROR_MYSQL_GENERIC_ERROR(mySQLException.getErrorCode(), mySQLException.what()));
    }

    cout<<SUCCESS_MYSQL_DB_CONNECTED<<'\n';

    return Server::Start(serverPort, SpecializedServer::ClientConnected_EventCallback);
}

SuccessState SpecializedServer::Stop()
{
    SuccessState successState = Server::Stop();
    if (!successState.isSuccess_Get())
        return successState;

    while (!pthread_mutex_trylock(&SpecializedServer::mySQLConnectionMutex));
    if (SpecializedServer::mySQLConnection != nullptr)
        delete SpecializedServer::mySQLConnection;
    pthread_mutex_unlock(&SpecializedServer::mySQLConnectionMutex);

    pthread_mutex_destroy(&SpecializedServer::mySQLConnectionMutex);
    pthread_mutex_destroy(&SpecializedServer::consoleMutex);

    return successState;
}

void SpecializedServer::ClientConnected_EventCallback(ClientSocket clientSocket)
{
    Statement * mySQLStatement;
    ResultSet * mySQLResultSet;
    bool isWhitelistedIP = false;

    try
    {
        mySQLStatement = SpecializedServer::mySQLConnection->createStatement();
        mySQLResultSet = mySQLStatement->executeQuery(MYSQL_GET_WHITELISTED_IP_COUNT_QUERY(clientSocket.clientIP));

        if (mySQLResultSet->next())
            isWhitelistedIP = (mySQLResultSet->getInt(1) > 0);
    }
    catch (SQLException & mySQLException)
    {
        cout<<ERROR_MYSQL_GENERIC_ERROR(mySQLException.getErrorCode(), mySQLException.what())<<'\n';

        if (mySQLStatement != nullptr)
            delete mySQLStatement;
        if (mySQLResultSet != nullptr)
            delete mySQLResultSet;
    }

    if (mySQLStatement != nullptr)
        delete mySQLStatement;
    if (mySQLResultSet != nullptr)
        delete mySQLResultSet;

    if (!isWhitelistedIP)
    {
        for (size_t clientSocketsIterator = 0; clientSocketsIterator < SpecializedServer::clientSockets.size(); ++clientSocketsIterator)
            if (SpecializedServer::clientSockets[clientSocketsIterator].clientIP == clientSocket.clientIP)
            {
                while (!pthread_mutex_trylock(&SpecializedServer::clientSocketsMutex));
                close(clientSocket.clientSocketDescriptor);
                
                SpecializedServer::clientSockets[clientSocketsIterator] = SpecializedServer::clientSockets[SpecializedServer::clientSockets.size() - 1];
                SpecializedServer::clientSockets.pop_back();
                pthread_mutex_unlock(&SpecializedServer::clientSocketsMutex);
            }

        cout<<ERROR_CLIENT_NOT_WHITELISTED(clientSocket.clientIP)<<'\n';
        return;
    }

    cout<<SUCCESS_CLIENT_CONNECTED(clientSocket.clientIP)<<'\n';
}