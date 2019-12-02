#include "../Headers/LocalClient.hpp"

bool LocalClient::serverStopped = false;

SuccessState LocalClient::StopServer()
{
    if (LocalClient::serverStopped)
        return SuccessState(false, ERROR_NO_SERVER_INSTANCE_RUNNING);

    Statement * mySQLStatement;
    ResultSet * mySQLResultSet;
    string localServerPath = INVALID_SERVER_PATH;

    try
    {
        Connection * mySQLConnection = MySQLConnector::mySQLConnection_Get();

        mySQLStatement = mySQLConnection->createStatement();
        mySQLResultSet = mySQLStatement->executeQuery(MYSQL_GET_LOCAL_SERVER_PATH_QUERY);

        if (mySQLResultSet->next())
            localServerPath = mySQLResultSet->getString(1);
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

    
    
    LocalClient::serverStopped = true;

    return SuccessState(true, SUCCESS_SERVER_STOPPED);
}