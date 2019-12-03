#pragma once

#define COMMAND_STOP_SERVER                 "STOP"
#define COMMAND_SUCCESS                     "SUCCESS"
#define COMMAND_FAILURE                     "FAILURE"
#define SUCCESS_SERVER_STOPPED              "Server successfully stopped"
#define ERROR_SERVER_STOPPED                "Failed to stop the server"
#define ERROR_NO_SERVER_INSTANCE_RUNNING    "There is no instance of the server currently running"
#define ERROR_GET_LOCAL_SERVER_PATH         "Could not retrieve local server path from the database"
#define ERROR_CLIENT_SOCKET_INITIALIZATION  "Could not initialize local client socket"
#define ERROR_LOCAL_SERVER_CONNECTION       "Failed to connect to the local server"
#define MYSQL_GET_LOCAL_SERVER_PATH_QUERY   "CALL sp_GetLocalServerPath();"
#define INVALID_SERVER_PATH                 ""

#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <iostream>
#include <algorithm>
#include "MySQLConnector.hpp"
#include "Encryption.hpp"
#include "SuccessState.hpp"

using namespace std;
using namespace sql;

class LocalClient
{
    public:  static SuccessState StopServer();

    private: static bool serverStopped;
};