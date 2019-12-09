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
            return encryptedValuePair.originalValue;

    return INVALID_STRING;
}

Encryption::CharArray Encryption::Vigenere::Encrypt(string inputString, string keyString, size_t randomPrefixLength, size_t randomSuffixlength)
{
    default_random_engine randomEngine;
    uniform_int_distribution<char> uniformRandomDistribution(numeric_limits<char>::min(), numeric_limits<char>::max());

    Encryption::CharArray encrpytedCharArray;
    encrpytedCharArray.charArray = new char[encrpytedCharArray.charArrayLength];

    size_t charArrayIterator = 0;
    for (size_t randomPrefixIterator = 0; randomPrefixIterator < randomPrefixLength; ++randomPrefixIterator)
    {
        encrpytedCharArray.charArray[charArrayIterator] = uniformRandomDistribution(randomEngine);

        ++charArrayIterator;
    }

    string originalKeyString = keyString;
    while (keyString.size() < inputString.size())
        keyString += originalKeyString;

    for (size_t inputStringIterator = 0; inputStringIterator <= inputString.size(); ++inputStringIterator)
    {
        encrpytedCharArray.charArray[charArrayIterator] = (char)((inputString[inputStringIterator] + keyString[inputStringIterator]) % (numeric_limits<char>::max() + 1));

        ++charArrayIterator;
    }

    for (size_t randomSuffixIterator = 0; randomSuffixIterator < randomSuffixlength; ++randomSuffixIterator)
    {
        encrpytedCharArray.charArray[charArrayIterator] = uniformRandomDistribution(randomEngine);

        ++charArrayIterator;
    }

    encrpytedCharArray.charArrayLength = charArrayIterator;

    return encrpytedCharArray;
}

string Encryption::Vigenere::Decrypt(Encryption::CharArray encrpytedString, string keyString, size_t randomPrefixLength, size_t randomSuffixlength)
{
    char * decrpytedString = new char[encrpytedString.charArrayLength - randomPrefixLength - randomSuffixlength];
    size_t decrpytedStringLength = 0;
    for (size_t encrpytedStringIterator = randomPrefixLength + 1; encrpytedStringIterator < encrpytedString.charArrayLength - randomSuffixlength; ++encrpytedStringIterator)
    {
        decrpytedString[decrpytedStringLength] = encrpytedString.charArray[encrpytedStringIterator];

        ++decrpytedStringLength;
    }

    string originalKeyString = keyString;
    while (keyString.size() < decrpytedStringLength)
        keyString += originalKeyString;

    for (size_t decryptedStringIterator = 0; decryptedStringIterator < decrpytedStringLength; ++decryptedStringIterator)
        decrpytedString[decryptedStringIterator] = (char)((decrpytedString[decryptedStringIterator] - keyString[decryptedStringIterator]) % (numeric_limits<char>::max() + 1));

    return decrpytedString;
}

Encryption::EncryptedValuePair::EncryptedValuePair()
{
}

Encryption::EncryptedValuePair::EncryptedValuePair(string originalValue, string encryptedValue) : originalValue(originalValue), encryptedValue(encryptedValue)
{
}

Encryption::CharArray::CharArray()
{
}

Encryption::CharArray::CharArray(char * charArray, size_t charArrayLength) : charArray(charArray), charArrayLength(charArrayLength)
{
}