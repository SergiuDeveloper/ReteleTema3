#include "../Headers/Encryption.hpp"

string Encryption::Algorithms::SHA256::Encrypt(string inputString)
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

string Encryption::Algorithms::SHA256::Decrypt(string encryptedString, vector<Encryption::Types::EncryptedValuePair> encryptedValuePairs)
{
    for (auto & encryptedValuePair : encryptedValuePairs)
        if (encryptedValuePair.encryptedValue == encryptedString)
            return encryptedValuePair.originalValue;

    return INVALID_STRING;
}

Encryption::Types::CharArray Encryption::Algorithms::Vigenere::Encrypt(string inputString, string keyString, size_t randomPrefixLength, size_t randomSuffixlength)
{
    long randomEngineSeed = std::chrono::system_clock::now().time_since_epoch().count();

    default_random_engine randomEngine(randomEngineSeed);
    uniform_int_distribution<char> uniformRandomDistribution(numeric_limits<char>::min(), numeric_limits<char>::max());

    Encryption::Types::CharArray encrpytedCharArray;
    encrpytedCharArray.charArrayLength = inputString.size() + randomPrefixLength + randomSuffixlength;
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

    char encrpytedCharacter;
    for (size_t inputStringIterator = 0; inputStringIterator <= inputString.size(); ++inputStringIterator)
    {
        encrpytedCharacter = (char)(inputString[inputStringIterator] + keyString[inputStringIterator]);
        encrpytedCharArray.charArray[charArrayIterator] = (encrpytedCharacter > numeric_limits<char>::max() ? numeric_limits<char>::min() + encrpytedCharacter - numeric_limits<char>::max() - 1 : encrpytedCharacter);

        ++charArrayIterator;
    }

    for (size_t randomSuffixIterator = 0; randomSuffixIterator < randomSuffixlength; ++randomSuffixIterator)
    {
        encrpytedCharArray.charArray[charArrayIterator] = uniformRandomDistribution(randomEngine);

        ++charArrayIterator;
    }

    return encrpytedCharArray;
}

string Encryption::Algorithms::Vigenere::Decrypt(Encryption::Types::CharArray encrpytedString, string keyString, size_t randomPrefixLength, size_t randomSuffixlength)
{
    char * decrpytedString = new char[encrpytedString.charArrayLength - randomPrefixLength - randomSuffixlength + 1];
    size_t decrpytedStringLength = 0;
    for (size_t encrpytedStringIterator = randomPrefixLength; encrpytedStringIterator < encrpytedString.charArrayLength - randomSuffixlength; ++encrpytedStringIterator)
    {
        decrpytedString[decrpytedStringLength] = encrpytedString.charArray[encrpytedStringIterator];

        ++decrpytedStringLength;
    }

    string originalKeyString = keyString;
    while (keyString.size() < decrpytedStringLength)
        keyString += originalKeyString;

    char decryptedCharacter;
    for (size_t decryptedStringIterator = 0; decryptedStringIterator < decrpytedStringLength; ++decryptedStringIterator)
    {
        decryptedCharacter = (char)(decrpytedString[decryptedStringIterator] - keyString[decryptedStringIterator]);
        decrpytedString[decryptedStringIterator] = (decryptedCharacter < numeric_limits<char>::min() ? decryptedCharacter + numeric_limits<char>::max() : decryptedCharacter);
    }
    decrpytedString[decrpytedStringLength] = '\0';

    return decrpytedString;
}

Encryption::Types::EncryptedValuePair::EncryptedValuePair()
{
}

Encryption::Types::EncryptedValuePair::EncryptedValuePair(string originalValue, string encryptedValue) : originalValue(originalValue), encryptedValue(encryptedValue)
{
}

Encryption::Types::CharArray::CharArray()
{
}

Encryption::Types::CharArray::CharArray(char * charArray, size_t charArrayLength) : charArray(charArray), charArrayLength(charArrayLength)
{
}