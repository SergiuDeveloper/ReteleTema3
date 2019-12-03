#include "../Headers/Encryption.hpp"

string Encryption::SHA256::Encrypt(string inputString)
{
    unsigned char hashedValue[SHA256_DIGEST_LENGTH];

    SHA256_CTX sha256State;
    SHA256_Init(&sha256State);
    SHA256_Update(&sha256State, inputString.c_str(), inputString.size());
    SHA256_Final(hashedValue, &sha256State);

    stringstream encryptedStringStream;
    for(int hashedValueIterator = 0; hashedValueIterator < SHA256_DIGEST_LENGTH; ++hashedValueIterator)
        encryptedStringStream << hex << setw(2) << setfill('0') << (int)hashedValue[hashedValueIterator];

    string encryptedString = encryptedStringStream.str();

    return encryptedString;
}

string Encryption::SHA256::Decrypt(string encryptedString, vector<Encryption::EncryptedValuePair> encryptedValuePairs)
{
    for (auto & encryptedValuePair : encryptedValuePairs)
        if (encryptedValuePair.encryptedValue == encryptedString)
            return encryptedString;

    return INVALID_STRING;
}

Encryption::EncryptedValuePair::EncryptedValuePair(string originalValue, string encryptedValue) : originalValue(originalValue), encryptedValue(encryptedValue)
{
}