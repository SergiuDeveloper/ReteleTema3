#pragma once

#define INVALID_STRING ""

#include <openssl/sha.h>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>

using namespace std;

class Encryption
{
    public: class EncryptedValuePair;

    public:
    class SHA256
    {
        public: static string Encrypt(string inputString);
        public: static string Decrypt(string encryptedString, vector<Encryption::EncryptedValuePair> encryptedValuePairs);
    };

    public:
    class EncryptedValuePair
    {
        public: EncryptedValuePair();
        public: EncryptedValuePair(string originalValue, string encryptedValue);

        public: string originalValue;
        public: string encryptedValue;
    };
};