#include "../Headers/MySQLConnector.hpp"

Driver * MySQLConnector::mySQLDriver;
Connection * MySQLConnector::mySQLConnection;
pthread_mutex_t MySQLConnector::mySQLConnectionMutex;

SuccessState MySQLConnector::Initialize(string mySQLSuperuserPassword)
{
    pthread_mutex_init(&MySQLConnector::mySQLConnectionMutex, nullptr);

    try
    {
        MySQLConnector::mySQLDriver = get_driver_instance();
        MySQLConnector::mySQLConnection = MySQLConnector::mySQLDriver->connect(MYSQL_CONNECTION_STRING(MYSQL_SERVER_PROTOCOL, MYSQL_SERVER_IP, MYSQL_SERVER_PORT), MYSQL_SERVER_SUPERUSER, mySQLSuperuserPassword);
        MySQLConnector::mySQLConnection->setSchema(MYSQL_DATABASE_DEFAULT_SCHEMA);
    }
    catch (SQLException & mySQLException)
    {
        delete MySQLConnector::mySQLConnection;
        return SuccessState(false, ERROR_MYSQL_GENERIC_ERROR(mySQLException.getErrorCode(), mySQLException.what()));
    }
    
    return SuccessState(true, SUCCESS_MYSQL_DB_CONNECTED);
}

void MySQLConnector::Deinitialize()
{
    while (!pthread_mutex_trylock(&MySQLConnector::mySQLConnectionMutex));
    if (MySQLConnector::mySQLConnection != nullptr)
    {
        MySQLConnector::mySQLConnection->close();
        delete MySQLConnector::mySQLConnection;
    }
    if (MySQLConnector::mySQLDriver != nullptr)
        MySQLConnector::mySQLDriver->threadEnd();
    pthread_mutex_unlock(&MySQLConnector::mySQLConnectionMutex);

    pthread_mutex_destroy(&MySQLConnector::mySQLConnectionMutex);
}

Connection * MySQLConnector::mySQLConnection_Get()
{
    return MySQLConnector::mySQLConnection;
}