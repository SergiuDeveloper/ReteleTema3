#pragma once

#define COMMAND_STOP_SERVER                     "STOP"
#define COMMAND_SUCCESS                         "SUCCESS"
#define COMMAND_FAILURE                         "FAILURE"
#define LOCAL_SERVER_PATH                       "/tmp/LocalServer2"
#define ERROR_MYSQL_LOG_LOCAL_SERVER_PATH       "Failed to log local server path in database"
#define ERROR_UNKNOWN_COMMAND_RECEIVED          "An unknown command was received from a local client"
#define MYSQL_UPDATE_LOCAL_SERVER_PATH_QUERY    "CALL sp_LogLocalServerPath(?);"

#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include <iostream>
#include <string>
#include "Server.hpp"
#include "SpecializedServer.hpp"
#include "MySQLConnector.hpp"
#include "Encryption.hpp"

using namespace std;
using namespace sql;

class LocalServer : public Server
{
    using Server::Start;
    using Server::Stop;

    private: LocalServer();

    public:  SuccessState Start();
    public:  SuccessState Stop();

    private: void ClientConnected_EventCallback(ClientSocket clientSocket);
    
    private: static LocalServer * singletonInstance;
    private: static pthread_mutex_t singletonInstanceMutex;
    public:  static const LocalServer * GetSingletonInstance();
};