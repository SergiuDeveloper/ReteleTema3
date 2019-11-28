#pragma once

#define SUCCESS_MYSQL_DB_CONNECTED                                                      "Successfully connected to MySQL database!"
#define SUCCESS_CLIENT_CONNECTED(clientIP)                                              ((string)"Client " + clientIP + " successfully connected!")
#define ERROR_MYSQL_DB_CONNECTION                                                       "Failed to connect to MySQL database! Check your credentials!"
#define PROMPT_MYSQL_DB_CREDENTIALS                                                     "Please enter the password for the MySQL database superuser(root)"
#define MYSQL_SERVER_PROTOCOL                                                           "tcp"
#define MYSQL_SERVER_IP                                                                 "localhost"
#define MYSQL_SERVER_PORT                                                               3306
#define MYSQL_SERVER_SUPERUSER                                                          "root"
#define MYSQL_CONNECTION_STRING(mySQLServerProtocol, mySQLServerIP, mySQLServerPort)    ((string)mySQLServerProtocol + "://" + mySQLServerIP + ":" + to_string(mySQLServerPort))

#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <ncurses.h>
#include <iostream>
#include <string>
#include "Server.hpp"
#include "ClientSocket.hpp"
#include "SuccessState.hpp"

using namespace std;
using namespace sql;

class SpecializedServer : public Server
{
    public:  static SuccessState Start(unsigned int serverPort);
    public:  static SuccessState Stop();
    
    private: static void ClientConnected_EventCallback(ClientSocket clientSocket);

    private: static Connection * mySQLConnection;
    private: static pthread_mutex_t mySQLConnectionMutex;
};