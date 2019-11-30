#pragma once

#define SUCCESS_MYSQL_DB_CONNECTED                                                      "Successfully connected to MySQL database!"
#define SUCCESS_CLIENT_CONNECTED(clientIP)                                              ((string)"Client " + clientIP + " successfully connected!")
#define ERROR_CLIENT_NOT_WHITELISTED(clientIP)                                          ((string)"Could not connect client " + clientIP + " to the server, as he is not whitelisted!")
#define ERROR_MYSQL_GENERIC_ERROR(mySQLErrorCode, mySQLExceptionMessage)                ((string)"Failed to connect to MySQL database! Runtime error " + to_string(mySQLErrorCode) + ": \"" + mySQLExceptionMessage + "\"")
#define PROMPT_MYSQL_DB_CREDENTIALS                                                     "Please enter the password for the MySQL database superuser(root): "
#define MYSQL_SERVER_PROTOCOL                                                           "tcp"
#define MYSQL_SERVER_IP                                                                 "localhost"
#define MYSQL_SERVER_PORT                                                               3306
#define MYSQL_SERVER_SUPERUSER                                                          "root"
#define MYSQL_DATABASE_DEFAULT_SCHEMA                                                   "sys"
#define MYSQL_CONNECTION_STRING(mySQLServerProtocol, mySQLServerIP, mySQLServerPort)    ((string)mySQLServerProtocol + "://" + mySQLServerIP + ":" + to_string(mySQLServerPort))
#define MYSQL_GET_WHITELISTED_IP_COUNT_QUERY(clientIP)                                  ((string)"SELECT COUNT(*) FROM ClientIPsWhitelist WHERE ClientIP = \"" + clientIP + "\";")

#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include <pthread.h>
#include <iostream>
#include <string>
#include "Server.hpp"
#include "ClientSocket.hpp"
#include "SuccessState.hpp"

using namespace std;
using namespace sql;

class SpecializedServer : public Server
{
    using Server::Start;
    using Server::Stop;

    public:  SuccessState Start(unsigned int serverPort);
    public:  SuccessState Stop();
    
    private: void ClientConnected_EventCallback(ClientSocket clientSocket);

    private: pthread_mutex_t consoleMutex;
    private: Connection * mySQLConnection;
    private: pthread_mutex_t mySQLConnectionMutex;

    private: static SpecializedServer * singletonInstance;
    public:  static const SpecializedServer * GetSingletonInstance();
};