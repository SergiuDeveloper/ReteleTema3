#include <iostream>
#include "Headers/Client.hpp"
#include "Headers/Encryption.hpp"

using namespace std;

int main()
{
    Client * client = (Client *)Client::GetSingletonInstance();
    SuccessState successState = client->Connect("127.0.0.1", 27015);

    cout<<successState.successStateMessage_Get()<<endl;

    while (client->isConnected_Get());

    return successState.isSuccess_Get() ? EXIT_SUCCESS : EXIT_FAILURE;
}