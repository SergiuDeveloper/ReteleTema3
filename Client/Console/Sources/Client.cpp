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

    this->clientMAC = Client::GetMacAddress();
    for (auto & macAddressIterator : this->clientMAC)
        macAddressIterator = toupper(macAddressIterator);

    string macAddressEncrypted = Encryption::Algorithms::SHA256::Encrypt(this->clientMAC);
    size_t macAddressEncryptedLength = macAddressEncrypted.size();
    
    operationSuccess = false;
    
    operationSuccess = (write(this->serverSocket, &macAddressEncryptedLength, sizeof(size_t)) > 0);
    if (!operationSuccess)
    {
        close(this->serverSocket);
        return SuccessState(false, ERROR_CONNECTION_INTRERUPTED);
    }
    operationSuccess = (write(this->serverSocket, macAddressEncrypted.c_str(), macAddressEncryptedLength) > 0);
    if (!operationSuccess)
    {
        close(this->serverSocket);
        return SuccessState(false, ERROR_CONNECTION_INTRERUPTED);
    }

    string adminNameEncrypted = Encryption::Algorithms::SHA256::Encrypt(administratorCredentials.adminName);
    string adminPasswordEncrypted = Encryption::Algorithms::SHA256::Encrypt(administratorCredentials.adminPassword);
    size_t adminNameEncryptedLength = adminNameEncrypted.size();
    size_t adminPasswordEncryptedLength = adminPasswordEncrypted.size();

    operationSuccess = (write(this->serverSocket, &adminNameEncryptedLength, sizeof(size_t)) > 0);
    if (!operationSuccess)
    {
        close(this->serverSocket);
        return SuccessState(false, ERROR_CONNECTION_INTRERUPTED);
    }
    operationSuccess = (write(this->serverSocket, adminNameEncrypted.c_str(), adminNameEncryptedLength) > 0);
    if (!operationSuccess)
    {
        close(this->serverSocket);
        return SuccessState(false, ERROR_CONNECTION_INTRERUPTED);
    }

    operationSuccess = (write(this->serverSocket, &adminPasswordEncryptedLength, sizeof(size_t)) > 0);
    if (!operationSuccess)
    {
        close(this->serverSocket);
        return SuccessState(false, ERROR_CONNECTION_INTRERUPTED);
    }
    operationSuccess = (write(this->serverSocket, adminPasswordEncrypted.c_str(), adminPasswordEncryptedLength) > 0);
    if (!operationSuccess)
    {
        close(this->serverSocket);
        return SuccessState(false, ERROR_CONNECTION_INTRERUPTED);
    }

    size_t serverResponseLength;
    operationSuccess = (read(this->serverSocket, &serverResponseLength, sizeof(size_t)) > 0);
    if (!operationSuccess)
    {
        close(this->serverSocket);
        return SuccessState(false, ERROR_CONNECTION_INTRERUPTED);
    }

    char * serverResponse = new char[serverResponseLength + 1];
    operationSuccess = (read(this->serverSocket, serverResponse, serverResponseLength) > 0);
    serverResponse[serverResponseLength] = '\0';
    if (!operationSuccess)
    {
        close(this->serverSocket);
        return SuccessState(false, ERROR_CONNECTION_INTRERUPTED);
    }

    string successMessageEncrypted = Encryption::Algorithms::SHA256::Encrypt(MESSAGE_SUCCESS);
    if (successMessageEncrypted != (string)serverResponse)
    {
        close(this->serverSocket);
        return SuccessState(false, ERROR_CONNECTION_INTRERUPTED);
    }

    delete serverResponse;

    this->isConnected = true;

    cout<<SUCCESS_CONNECTION_ESTABLISHED(this->serverIP, this->serverPort)<<endl;

    char * clientMACCString = new char[this->clientMAC.size() + 6];
    sprintf(clientMACCString, "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c", this->clientMAC[0], this->clientMAC[1], this->clientMAC[2], this->clientMAC[3], this->clientMAC[4], this->clientMAC[5], this->clientMAC[6], this->clientMAC[7],
        this->clientMAC[8], this->clientMAC[9], this->clientMAC[10], this->clientMAC[11]);
    
    this->clientMAC = clientMACCString;

    Encryption::Types::CharArray encryptedResult;

    string currentUser = INVALID_STRING;
    string commandExecutionLocation = INVALID_STRING;

    int readResult = (read(this->serverSocket, &encryptedResult.charArrayLength, sizeof(size_t)));
    if (readResult <= 0)
    {
        cout<<ERROR_COMMAND_EXECUTION<<endl;

        if (readResult == 0)
        {
            this->Disconnect();
            return SuccessState(false, ERROR_CONNECTION_INTRERUPTED);
        }
    }
    else
    {   
        encryptedResult.charArray = new char[encryptedResult.charArrayLength];

        readResult = (read(this->serverSocket, encryptedResult.charArray, encryptedResult.charArrayLength));
        if (readResult <= 0)
        {
            cout<<ERROR_COMMAND_EXECUTION<<endl;

            if (readResult == 0)
            {
                this->Disconnect();
                return SuccessState(false, ERROR_CONNECTION_INTRERUPTED);
            }
        }
        else
        {
            string receivedResult = Encryption::Algorithms::Vigenere::Decrypt(encryptedResult, VIGENERE_KEY(this->serverPort, this->clientMAC), VIGENERE_RANDOM_PREFIX_LENGTH, VIGENERE_RANDOM_SUFFIX_LENGTH);

            size_t newlinePosition = receivedResult.find('\n');
            commandExecutionLocation = receivedResult.substr(0, newlinePosition);
            receivedResult = receivedResult.erase(0, newlinePosition + 1);

            newlinePosition = receivedResult.find('\n');
            currentUser = receivedResult.substr(0, newlinePosition);
            receivedResult = receivedResult.erase(0, newlinePosition + 1);

            INITIALIZE_COLOR();
            
            cout<<COLOR_GREEN<<currentUser<<COLOR_DEFAULT<<':';
            cout<<COLOR_BLUE<<commandExecutionLocation<<COLOR_DEFAULT<<"$ ";
        }
    }

    this->ClientLifecycle(currentUser, commandExecutionLocation);

    return SuccessState(true, SUCCESS_CLIENT_LIFECYCLE);
}

SuccessState Client::Disconnect()
{
    if (!this->isConnected)
        return SuccessState(false, ERROR_NO_CONNECTION_ESTABLISHED);

    close(this->serverSocket);

    this->isConnected = false;

    return SuccessState(true, SUCCESS_CONNECTION_CLOSED(this->serverIP, this->serverPort));
}

void Client::ClientLifecycle(string currentUser, string commandExecutionLocation)
{
    string quitCommand = COMMAND_QUIT_CLIENT;
    string helpCommand = COMMAND_HELP;

    int readResult;
    SuccessState successState(false, INVALID_STRING);
    string requestCommand;
    Encryption::Types::CharArray requestCommandCharArray;
    bool isQuitCommand;
    bool isHelpCommand;
    while (this->isConnected)
    {
        successState = this->GetCommandToSend();

        if (successState.successStateMessage_Get() == INVALID_STRING)
            continue;

        if (!successState.isSuccess_Get())
        {
            cout<<successState.successStateMessage_Get();
            continue;
        }

        requestCommand = successState.successStateMessage_Get();

        if (requestCommand.size() == quitCommand.size())
        {
            isQuitCommand = true;
            for (size_t requestCommandIterator = 0; requestCommandIterator < requestCommand.size(); ++requestCommandIterator)
                if (requestCommand[requestCommandIterator] != quitCommand[requestCommandIterator] && toupper(requestCommand[requestCommandIterator]) != quitCommand[requestCommandIterator])
                    isQuitCommand = false;

            if (isQuitCommand)
            {
                this->Disconnect();
                return;
            }
        }

        if (requestCommand.size() == helpCommand.size())
        {
            isHelpCommand = true;
             for (size_t requestCommandIterator = 0; requestCommandIterator < requestCommand.size(); ++requestCommandIterator)
                if (requestCommand[requestCommandIterator] != helpCommand[requestCommandIterator] && toupper(requestCommand[requestCommandIterator]) != helpCommand[requestCommandIterator])
                    isHelpCommand = false;

            if (isHelpCommand)
            {
                cout<<MESSAGE_COMMANDS_USAGE<<endl;

                cout<<COLOR_GREEN<<currentUser<<COLOR_DEFAULT<<':';
                cout<<COLOR_BLUE<<commandExecutionLocation<<COLOR_DEFAULT<<"$ ";
                
                continue;
            }
        }

        requestCommandCharArray = Encryption::Algorithms::Vigenere::Encrypt(requestCommand, VIGENERE_KEY(this->serverPort, this->clientMAC), VIGENERE_RANDOM_PREFIX_LENGTH, VIGENERE_RANDOM_SUFFIX_LENGTH);
        
        write(this->serverSocket, &requestCommandCharArray.charArrayLength, sizeof(size_t));
        write(this->serverSocket, requestCommandCharArray.charArray, requestCommandCharArray.charArrayLength);

        Encryption::Types::CharArray encryptedResult;

        readResult = (read(this->serverSocket, &encryptedResult.charArrayLength, sizeof(size_t)));
        if (readResult <= 0)
        {
            cout<<ERROR_COMMAND_EXECUTION<<endl;

            if (readResult == 0)
            {
                this->Disconnect();
                return;
            }
            continue;
        }
        
        encryptedResult.charArray = new char[encryptedResult.charArrayLength + 1];

        readResult = (read(this->serverSocket, encryptedResult.charArray, encryptedResult.charArrayLength));
        if (readResult <= 0)
        {
            cout<<ERROR_COMMAND_EXECUTION<<endl;

            if (readResult == 0)
            {
                this->Disconnect();
                return;
            }
            continue;
        }

        encryptedResult.charArray[encryptedResult.charArrayLength] = '\0';

        string receivedResult = Encryption::Algorithms::Vigenere::Decrypt(encryptedResult, VIGENERE_KEY(this->serverPort, this->clientMAC), VIGENERE_RANDOM_PREFIX_LENGTH, VIGENERE_RANDOM_SUFFIX_LENGTH);

        size_t newlinePosition = receivedResult.find('\n');
        if (newlinePosition == string::npos)
        {
            currentUser = receivedResult;
            receivedResult.clear();
        }
        else
        {
            currentUser = receivedResult.substr(0, newlinePosition);
            receivedResult = receivedResult.erase(0, newlinePosition + 1);
        }

        
        commandExecutionLocation = receivedResult;
        while (commandExecutionLocation[commandExecutionLocation.size() - 1] == '\n')
            commandExecutionLocation.pop_back();

        newlinePosition = commandExecutionLocation.rfind('\n');
        commandExecutionLocation = commandExecutionLocation.substr(newlinePosition + 1);
        receivedResult.erase(newlinePosition + 1);

        cout<<receivedResult;
        cout<<COLOR_GREEN<<currentUser<<COLOR_DEFAULT<<':';
        cout<<COLOR_BLUE<<commandExecutionLocation<<COLOR_DEFAULT<<"$ ";
    }
}

SuccessState Client::GetCommandToSend()
{
    if (!this->isConnected)
        return SuccessState(false, ERROR_NO_CONNECTION_ESTABLISHED);

    string commandToSend;
    getline(cin, commandToSend);

    return SuccessState(true, commandToSend);
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
    const string macAddressFormat = "%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX";
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

const string Client::clientMAC_Get()
{
    return this->clientMAC;
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