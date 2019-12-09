#pragma once

#define INVALID_STRING ""

#include <openssl/sha.h>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <random>

using namespace std;

class Encryption
{
    public: class EncryptedValuePair;
    public: class CharArray;

    public:
    class SHA256
    {
        public: static string Encrypt(string inputString);
        public: static string Decrypt(string encryptedString, vector<Encryption::EncryptedValuePair> encryptedValuePairs);
    };

    public:
    class Vigenere
    {
        public: static Encryption::CharArray Encrypt(string inputString, string keyString, size_t randomPrefixLength, size_t randomSuffixlength);
        public: static string Decrypt(CharArray encrpytedString, string keyString, size_t randomPrefixLength, size_t randomSuffixlength);
    };

    public:
    class EncryptedValuePair
    {
        public: EncryptedValuePair();
        public: EncryptedValuePair(string originalValue, string encryptedValue);

        public: string originalValue;
        public: string encryptedValue;
    };

    public:
    class CharArray
    {
        public: CharArray();
        public: CharArray(char * charArray, size_t charArrayLength);

        public: char * charArray;
        public: size_t charArrayLength;
    }
};