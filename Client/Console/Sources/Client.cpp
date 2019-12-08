#include "../Headers/Client.hpp"

Client * Client::singletonInstance;

Client::Client()
{
}

SuccessState Client::Connect(string serverIP, unsigned int serverPort)
{
    if (this->isConnected)
        return SuccessState(false, ERROR_CONNECTION_ALREADY_ESTABLISHED);

    this->isConnected = false;

    Client::AdministratorCredentials administratorCredentials = this->GetAdministratorCredentials();

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

    string macAddress = Client::GetMacAddress();
    for (auto & macAddressIterator : macAddress)
        macAddressIterator = toupper(macAddressIterator);

    string macAddressEncrypted = Encryption::SHA256::Encrypt(macAddress);
    size_t macAddressEncryptedLength = macAddressEncrypted.size();
    
    operationSuccess = false;

    operationSuccess = (write(serverSocket, &macAddressEncryptedLength, sizeof(size_t)) > 0);
    if (!operationSuccess)
    {
        close(serverSocket);
        return SuccessState(false, ERROR_CONNECTION_INTRERUPTED);
    }
    operationSuccess = (write(serverSocket, macAddressEncrypted.c_str(), macAddressEncryptedLength) > 0);
    if (!operationSuccess)
    {
        close(serverSocket);
        return SuccessState(false, ERROR_CONNECTION_INTRERUPTED);
    }

    string adminNameEncrypted = Encryption::SHA256::Encrypt(administratorCredentials.adminName);
    string adminPasswordEncrypted = Encryption::SHA256::Encrypt(administratorCredentials.adminPassword);
    size_t adminNameEncryptedLength = adminNameEncrypted.size();
    size_t adminPasswordEncryptedLength = adminPasswordEncrypted.size();

    operationSuccess = (write(serverSocket, &adminNameEncryptedLength, sizeof(size_t)) > 0);
    if (!operationSuccess)
    {
        close(serverSocket);
        return SuccessState(false, ERROR_CONNECTION_INTRERUPTED);
    }
    operationSuccess = (write(serverSocket, adminNameEncrypted.c_str(), adminNameEncryptedLength) > 0);
    if (!operationSuccess)
    {
        close(serverSocket);
        return SuccessState(false, ERROR_CONNECTION_INTRERUPTED);
    }

    operationSuccess = (write(serverSocket, &adminPasswordEncryptedLength, sizeof(size_t)) > 0);
    if (!operationSuccess)
    {
        close(serverSocket);
        return SuccessState(false, ERROR_CONNECTION_INTRERUPTED);
    }
    operationSuccess = (write(serverSocket, adminPasswordEncrypted.c_str(), adminPasswordEncryptedLength) > 0);
    if (!operationSuccess)
    {
        close(serverSocket);
        return SuccessState(false, ERROR_CONNECTION_INTRERUPTED);
    }

    bool testConnectionVariable;
    operationSuccess = read(serverSocket, &testConnectionVariable, sizeof(bool));
    if (!operationSuccess)
    {
        close(serverSocket);
        return SuccessState(false, ERROR_CONNECTION_INTRERUPTED);
    }

    this->isConnected = true;

    return SuccessState(true, SUCCESS_CONNECTION_ESTABLISHED(this->serverIP, this->serverPort));
}

SuccessState Client::Disconnect()
{
    if (!this->isConnected)
        return SuccessState(false, ERROR_NO_CONNECTION_ESTABLISHED);

    close(this->serverSocket);

    this->isConnected = false;

    return SuccessState(true, SUCCESS_CONNECTION_CLOSED(this->serverIP, this->serverPort));
}

Client::AdministratorCredentials Client::GetAdministratorCredentials()
{
    string adminName, adminPassword;

    cout<<PROMPT_ENTER_ADMINISTRATOR_USERNAME;
    cin>>adminName;

    adminPassword = getpass(PROMPT_ENTER_ADMINISTRATOR_PASSWORD);

    return AdministratorCredentials(adminName, adminPassword);
}

string Client::GetMacAddress()
{
    const string macAddressFormat = "%.2hhX:%.2hhX:%.2hhX:%.2hhX:%.2hhX:%.2hhX";
    const string macInterfaceToken = "eth0";

	struct ifreq macInterface;
	
	int macSocketDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
	macInterface.ifr_addr.sa_family = AF_INET;
	strcpy(macInterface.ifr_name, macInterfaceToken.c_str());

	ioctl(macSocketDescriptor, SIOCGIFHWADDR, &macInterface);

	close(macSocketDescriptor);
	
	string macAddressRaw = macInterface.ifr_hwaddr.sa_data;
    char macAddress[macAddressRaw.size() + 6];
    sprintf(macAddress, macAddressFormat.c_str(), macAddressRaw[0], macAddressRaw[1], macAddressRaw[2], macAddressRaw[3], macAddressRaw[4], macAddressRaw[5]);

    return macAddress;
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

Client::AdministratorCredentials::AdministratorCredentials(string adminName, string adminPassword) : adminName(adminName), adminPassword(adminPassword)
{
}