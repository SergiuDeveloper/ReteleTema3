#include "../Headers/SpecializedServer.hpp"

Connection * SpecializedServer::mySQLConnection;
pthread_mutex_t SpecializedServer::mySQLConnectionMutex;

SuccessState SpecializedServer::Start(unsigned int serverPort)
{
    string mySQLSuperuserPassword;
    cout<<PROMPT_MYSQL_DB_CREDENTIALS<<'\n';
    noecho();
    getline(cin, mySQLSuperuserPassword);
    echo();

    pthread_mutex_init(&SpecializedServer::mySQLConnectionMutex, nullptr);

    Driver * mySQLDriver = get_driver_instance();
    SpecializedServer::mySQLConnection = mySQLDriver->connect(MYSQL_CONNECTION_STRING(MYSQL_SERVER_PROTOCOL, MYSQL_SERVER_IP, MYSQL_SERVER_PORT), MYSQL_SERVER_SUPERUSER, mySQLSuperuserPassword);

    return Server::Start(serverPort, SpecializedServer::ClientConnected_EventCallback);
}

SuccessState SpecializedServer::Stop()
{
    SuccessState successState = Server::Stop();
    if (!successState.isSuccess_Get())
        return successState;

    while (!pthread_mutex_trylock(&SpecializedServer::mySQLConnectionMutex));
    delete SpecializedServer::mySQLConnection;
    pthread_mutex_unlock(&SpecializedServer::mySQLConnectionMutex);

    pthread_mutex_destroy(&SpecializedServer::mySQLConnectionMutex);

    return successState;
}

void SpecializedServer::ClientConnected_EventCallback(ClientSocket clientSocket)
{
    cout<<SUCCESS_CLIENT_CONNECTED(clientSocket.clientIP)<<'\n';
}