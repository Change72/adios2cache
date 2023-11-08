#include "CacheBitmap.h"
#include <iostream>
#include <fstream>

int main()
{
    CacheBitmap cacheBitmap;
    cacheBitmap.data.resize(1000, false); // Create a vector with 1000 boolean values

    cacheBitmap.data[0] = true;
    cacheBitmap.data[1] = false;
    cacheBitmap.data[999] = true;

    // Write the vector to a file
    std::ofstream out("boolVector.bin", std::ios::binary);
    std::vector<unsigned char> bytes = cacheBitmap.boolsToBytes(cacheBitmap.data);
    std::cout << "bytes.size() = " << bytes.size() << std::endl;
    std::cout << "bytes[0] = " << (int)bytes[0] << std::endl;
    for (int i = 0; i < bytes.size(); ++i) {
        out.write(reinterpret_cast<const char*>(&bytes[i]), sizeof(bytes[i]));
    }
    out.close();

    // Clear the vector
    cacheBitmap.data.clear();

    // Read the vector from the file
    std::ifstream in("boolVector.bin", std::ios::binary);
    unsigned char byte;
    std::vector<unsigned char> bytes2;
    while (in.read(reinterpret_cast<char*>(&byte), sizeof(byte))) {
        bytes2.push_back(byte);
    }
    std::cout << "bytes2.size() = " << bytes2.size() << std::endl;
    std::cout << "bytes2[0] = " << (int)bytes2[0] << std::endl;
    cacheBitmap.data = cacheBitmap.bytesToBools(bytes2);

    in.close();

    std::cout << "cacheBitmap.data.size() = " << cacheBitmap.data.size() << std::endl;
    // Print the boolVector
    for (size_t i = 0; i < cacheBitmap.data.size(); ++i) {
        std::cout << cacheBitmap.data[i] << " ";
    }
    std::cout << std::endl;

    CacheBitmap cacheBitmap2;
    std::cout << "cacheBitmap2.data.size() = " << cacheBitmap2.data.size() << std::endl;
    cacheBitmap2.data = cacheBitmap2.bytesToBools(bytes2);
    std::cout << "cacheBitmap2.data.size() = " << cacheBitmap2.data.size() << std::endl;
    // Print the boolVector
    for (size_t i = 0; i < cacheBitmap2.data.size(); ++i) {
        std::cout << cacheBitmap2.data[i] << " ";
    }
    std::cout << std::endl;

    return 0;
}

