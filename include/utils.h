#include <iostream>
#include <string>
#include <openssl/md5.h>

// size of key in bytes is 16  (128 bits)
std::string md5(const std::string& input) {
    // Create an MD5 context
    MD5_CTX ctx;
    MD5_Init(&ctx);

    // Update the context with the input data
    MD5_Update(&ctx, input.c_str(), input.length());

    // Calculate the MD5 hash
    unsigned char result[MD5_DIGEST_LENGTH];
    MD5_Final(result, &ctx);

    // Convert the hash to a hexadecimal string
    std::string hash;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        char hex[3];
        sprintf(hex, "%02x", result[i]);
        hash += hex;
    }

    return hash;
}

