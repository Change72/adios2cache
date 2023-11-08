#include "utils.h"

// g++ -o TestMd5 TestMd5.cpp -lssl -lcrypto -I/usr/include/openssl

int main() {
    std::string key = "your_key_here22222";
    std::string hash = md5(key);

    std::cout << "MD5 Hash: " << hash << std::endl;

    return 0;
}