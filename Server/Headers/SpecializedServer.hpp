#pragma once

#define HTTP_PORT                                                                       80
#define SUCCESS_CLIENT_CONNECTED(clientIP)                                              ((string)"Client " + clientIP + " successfully connected!")
#define MYSQL_CONNECTION_STRING(mySQLServerProtocol, mySQLServerIP, mySQLServerPort)    ((string)mySQLServerProtocol + "://" + mySQLServerIP + ":" + to_string(mySQLServerPort))
#define MYSQL_IS_WHITELISTED_CLIENT_QUERY                                               ((string)"CALL sp_IsWhiteListedClient(\"?\", \"?\");")

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

    public:  SuccessState Start();
    private: SuccessState Start(string serverPath);
    public:  SuccessState Stop();
    
    private: void ClientConnected_EventCallback(ClientSocket clientSocket);

    private: LocalServer * localServer;
    private: pthread_mutex_t consoleMutex;

    private: static SpecializedServer * singletonInstance;
    private: static pthread_mutex_t singletonInstanceMutex;
    public:  static const SpecializedServer * GetSingletonInstance();
};