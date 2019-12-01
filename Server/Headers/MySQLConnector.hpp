#pragma once

#define SUCCESS_MYSQL_DB_CONNECTED                                                      "Successfully connected to MySQL database!"
#define ERROR_CLIENT_NOT_WHITELISTED(clientIP)                                          ((string)"Could not connect client " + clientIP + " to the server, as he is not whitelisted!")
#define ERROR_MYSQL_GENERIC_ERROR(mySQLErrorCode, mySQLExceptionMessage)                ((string)"MySQL database error! Runtime error " + to_string(mySQLErrorCode) + ": \"" + mySQLExceptionMessage + "\"")
#define PROMPT_MYSQL_DB_CREDENTIALS                                                     "Please enter the password for the MySQL database superuser(root): "
#define MYSQL_SERVER_PROTOCOL                                                           "tcp"
#define MYSQL_SERVER_IP                                                                 "localhost"
#define MYSQL_SERVER_PORT                                                               3306
#define MYSQL_SERVER_SUPERUSER                                                          "root"
#define MYSQL_DATABASE_DEFAULT_SCHEMA                                                   "sys"
#define MYSQL_CONNECTION_STRING(mySQLServerProtocol, mySQLServerIP, mySQLServerPort)    ((string)mySQLServerProtocol + "://" + mySQLServerIP + ":" + to_string(mySQLServerPort))

#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <pthread.h>
#include "SuccessState.hpp"

using namespace sql;

class MySQLConnector
{
    public:  static SuccessState Initialize(string mySQLSuperuserPassword);
    public:  static void Deinitialize();

    private: static Driver * mySQLDriver;
    private: static Connection * mySQLConnection;
    private: static pthread_mutex_t mySQLConnectionMutex;

    public:  static Connection * mySQLConnection_Get();
};