#include <iostream>
#include <vector>

class CacheBitmap
{
public:
    // data
    std::vector<bool> data{};

    // size
    size_t size() const
    {
        return data.size();
    }

    // Function to convert a byte to a vector of bools
    std::vector<bool> byteToBools(unsigned char byte) {
        std::vector<bool> bools(8);
        for (int i = 0; i < 8; ++i) {
            bools[i] = (byte >> i) & 1;
        }
        return bools;
    }

    // Function to convert a vector of bools to a byte
    unsigned char boolsToByte(const std::vector<bool>& bools) {
        unsigned char byte = 0;
        for (int i = 0; i < bools.size(); ++i) {
            byte |= bools[i] << i;
        }
        return byte;
    }

    // Function to convert a list of bytes to a vector of bools
    std::vector<bool> bytesToBools(const std::vector<unsigned char>& bytes) {
        std::vector<bool> bools;
        for (int i = 0; i < bytes.size(); ++i) {
            std::vector<bool> byteBools = byteToBools(bytes[i]);
            bools.insert(bools.end(), byteBools.begin(), byteBools.end());
        }
        return bools;
    }

    // Function to convert a vector of bools to a list of bytes
    std::vector<unsigned char> boolsToBytes(const std::vector<bool>& bools) {
        std::vector<unsigned char> bytes(bools.size() / 8);
        for (int i = 0; i < bytes.size(); ++i) {
            std::vector<bool> byteBools(bools.begin() + i * 8, bools.begin() + i * 8 + 8);
            bytes[i] = boolsToByte(byteBools);
        }
        return bytes;
    }
};

