#include "../Headers/Client.hpp"

Client * Client::singletonInstance;

SuccessState Client::Connect(string serverIP, unsigned int serverPort)
{
    if (this->isConnected)
        return SuccessState(false, ERROR_CONNECTION_ALREADY_ESTABLISHED);

    this->serverIP = serverIP;
    this->serverPort = serverPort;

    bool operationSuccess;

    this->serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    operationSuccess = (this->serverSocket != -1);
    if (!operationSuccess)
        return SuccessState(false, ERROR_COULD_NOT_INITIALIZE_SOCKET);

    struct sockaddr_in serverSocketAddr;
    serverSocketAddr.sin_family = AF_INET;
    inet_pton(AF_INET, serverIP.c_str(), &serverSocketAddr.sin_addr);
    serverSocketAddr.sin_port = htons(serverPort);

    operationSuccess = (connect(this->serverSocket, (struct sockaddr *)&serverSocketAddr, sizeof(serverSocketAddr)) != -1);
    if (!operationSuccess)
        return SuccessState(false, ERROR_CONNECTION_FAILED(this->serverIP, this->serverPort));

    this->isConnected = true;

    return SuccessState(true, SUCCESS_CONNECTION_ESTABLISHED(this->serverIP, this->serverPort));
}

SuccessState Client::Disconnect()
{
    if (!this->isConnected)
        return SuccessState(false, ERROR_NO_CONNECTION_ESTABLISHED);

    close(this->serverSocket);

    this->isConnected = false;

    return SuccessState(true, SUCCESS_CONNECTION_CLOSED(this->ServerIP, this->serverPort));
}

const string Client::serverIP_Get()
{
    return this->serverIP;
}

const unsigned int Client::serverPort_Get()
{
    return this->serverPort;
}

const bool Client::isConnected_Get()
{
    return this->isConnected;
}

const Client * Client::GetSingletonInstance()
{
    if (Client::singletonInstance == nullptr)
        Client::singletonInstance = new Client();

    return Client::singletonInstance;
}