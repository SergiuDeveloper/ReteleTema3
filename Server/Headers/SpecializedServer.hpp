#pragma once

#define SUCCESS_CLIENT_CONNECTED(clientIP, clientMAC)                                   ((string)"Client " + clientIP + " : " + clientMAC + " successfully connected!")
#define ERROR_CLIENT_ALREADY_CONNECTED(clientIP, clientMAC)                             ((string)"Client " + clientIP + " : " + clientMAC + " : " + " is already connected!")
#define ERROR_CLIENT_NOT_WHITELISTED(clientIP)                                          ((string)"Could not connect client " + clientIP + " to the server, as he is not whitelisted!")
#define MYSQL_CONNECTION_STRING(mySQLServerProtocol, mySQLServerIP, mySQLServerPort)    ((string)mySQLServerProtocol + "://" + mySQLServerIP + ":" + to_string(mySQLServerPort))
#define MYSQL_IS_WHITELISTED_IP_QUERY                                                   ((string)"CALL sp_IsWhitelistedIP(?);")
#define MYSQL_GET_MACS_FOR_WHITELISTED_IP_QUERY                                         ((string)"CALL sp_GetMACsForWhitelistedIP(?);")
#define MYSQL_GET_ALL_ADMINISTRATOR_CREDENTIALS                                         ((string)"CALL sp_GetAllAdministratorCredentials();")
#define MESSAGE_SUCCESS                                                                 "SUCCESS"

#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include <pthread.h>
#include <iostream>
#include <string>
#include "Server.hpp"
#include "LocalServer.hpp"
#include "MySQLConnector.hpp"
#include "ClientSocket.hpp"
#include "SuccessState.hpp"

using namespace std;
using namespace sql;

class LocalServer;

class SpecializedServer : public Server
{
    using Server::Start;
    using Server::Stop;

    private: SpecializedServer();

    public:  SuccessState Start(unsigned int serverPort);
    public:  SuccessState Stop();
    
    private: void ClientConnected_EventCallback(ClientSocket clientSocket);

    private: LocalServer * localServer;
    private: pthread_mutex_t consoleMutex;

    private: static SpecializedServer * singletonInstance;
    private: static pthread_mutex_t singletonInstanceMutex;
    public:  static const SpecializedServer * GetSingletonInstance();
};