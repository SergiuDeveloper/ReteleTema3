#pragma once

#define LOCAL_SERVER_PATH                       "/tmp/LocalServer"
#define ERROR_MYSQL_LOG_LOCAL_SERVER_PATH       "Failed to log local server path in database"
#define MYSQL_UPDATE_LOCAL_SERVER_PATH_QUERY    "CALL sp_LogLocalServerPath(?);"

#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include <string>
#include "Server.hpp"
#include "MySQLConnector.hpp"

using namespace std;
using namespace sql;

class LocalServer : public Server
{
    using Server::Start;
    using Server::Stop;

    public:  SuccessState Start();
    private: SuccessState Start(unsigned int serverPort);
    private: SuccessState Start(string serverPath);
    public:  SuccessState Stop();

    private: void ClientConnected_EventCallback(ClientSocket clientSocket);
    
    private: static LocalServer * singletonInstance;
    private: static pthread_mutex_t singletonInstanceMutex;
    public:  static const LocalServer * GetSingletonInstance();
};