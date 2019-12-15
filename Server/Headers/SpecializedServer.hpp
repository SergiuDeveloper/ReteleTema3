#pragma once

#define SUCCESS_RDC_STREAMING_SERVER_STARTED                                            "Successfully started the RDC Streaming Server"
#define SUCCESS_RDC_STREAMING_SERVER_STOPPED                                            "Successfully stopped the RDC Streaming Server"
#define SUCCESS_ADDED_CLIENT_TO_RDC_STREAMING                                           "Successfully added client to the RDC Streaming Server"
#define SUCCESS_CLIENT_CONNECTED(clientIP, clientMAC)                                   ((string)"Client " + clientIP + " : " + clientMAC + " successfully connected")
#define SUCCESS_CLIENT_DISCONNECTED(clientIP, clientMAC)                                ((string)"Client " + clientIP + " : " + clientMAC + " successfully disconnected")
#define SUCCESS_RECEIVED_REQUEST(clientIP, clientMAC, requestString)                    ((string)"Received request from " + clientIP + " : " + clientMAC + " : \"" + requestString + "\"")
#define SUCCESS_EXECUTED_COMMAND(clientIP, clientMAC)                                   ((string)"Successfully executed request received from " + clientIP + " : " + clientMAC)
#define FAILURE_RDC_STREAMING_SERVER_STARTED                                            "Failed to start the RDC Streaming Server"
#define FAILURE_RDC_STREAMING_SERVER_STOPPED                                            "Failed to stop the RDC Streaming Server"
#define FAILURE_ADDED_CLIENT_TO_RDC_STREAMING                                           "Failed to add client to the RDC Streaming Server"
#define ERROR_CLIENT_ALREADY_CONNECTED(clientIP, clientMAC)                             ((string)"Client " + clientIP + " : " + clientMAC + " is already connected")
#define ERROR_CLIENT_NOT_WHITELISTED(clientIP)                                          ((string)"Could not connect client " + clientIP + " to the server, as he is not whitelisted")
#define ERROR_EXECUTE_COMMAND(clientIP, clientMAC)                                      ((string)"Failed to execute command received from " + clientIP + clientMAC)
#define MYSQL_CONNECTION_STRING(mySQLServerProtocol, mySQLServerIP, mySQLServerPort)    ((string)mySQLServerProtocol + "://" + mySQLServerIP + ":" + to_string(mySQLServerPort))
#define MYSQL_IS_WHITELISTED_IP_QUERY                                                   ((string)"CALL sp_IsWhitelistedIP(?);")
#define MYSQL_GET_MACS_FOR_WHITELISTED_IP_QUERY                                         ((string)"CALL sp_GetMACsForWhitelistedIP(?);")
#define MYSQL_GET_ALL_ADMINISTRATOR_CREDENTIALS                                         ((string)"CALL sp_GetAllAdministratorCredentials();")
#define MESSAGE_SUCCESS                                                                 "SUCCESS"
#define MESSAGE_FAILURE                                                                 "FAILURE"
#define MESSAGE_CONNECT_RDC                                                             "CONNECT RDC"
#define SOCKET_CHUNK_SIZE                                                               4096
#define DEFAULT_EXECUTION_PATH                                                          "./"
#define UNIX_USERNAME_ENV                                                               "USERNAME"
#define VIGENERE_KEY(serverPort, clientMAC)                                             (to_string(serverPort) + clientMAC)
#define VIGENERE_RANDOM_PREFIX_LENGTH                                                   32
#define VIGENERE_RANDOM_SUFFIX_LENGTH                                                   32

#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include <pthread.h>
#include <limits.h>
#include <iostream>
#include <string>
#include "Server.hpp"
#include "Encryption.hpp"
#include "LocalServer.hpp"
#include "RDCStreamingServer.hpp"
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
    private: void ClientRequest_EventCallback(ClientSocket clientSocket, Encryption::Types::CharArray messageToProcess, string & commandExecutionPath);
    private: void ClientDisconnected_EventCallback(ClientSocket clientSocket);

    private: LocalServer * localServer;
    private: pthread_mutex_t consoleMutex;

    private: static SpecializedServer * singletonInstance;
    private: static pthread_mutex_t singletonInstanceMutex;
    public:  static const SpecializedServer * GetSingletonInstance();
};