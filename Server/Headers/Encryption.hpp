#pragma once

#define INVALID_STRING ""

#include <openssl/sha.h>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <random>
#include <chrono>

using namespace std;

class Encryption
{
    public:
    class Types
    {
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
        };
    };

    public:
    class Algorithms
    {
        public:
        class SHA256
        {
            public: static string Encrypt(string inputString);
            public: static string Decrypt(string encryptedString, vector<Encryption::Types::EncryptedValuePair> encryptedValuePairs);
        };

        public:
        class Vigenere
        {
            public: static Encryption::Types::CharArray Encrypt(string inputString, string keyString, size_t randomPrefixLength, size_t randomSuffixlength);
            public: static string Decrypt(Encryption::Types::CharArray encrpytedString, string keyString, size_t randomPrefixLength, size_t randomSuffixlength);
        };
    };
};