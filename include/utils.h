#include <iostream>
#include <string>
#include <openssl/md5.h>
#include "QueryBox.h"

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


// For 2d or 3d, if a query box is interacted with another box, copy the data from the intersection
// row major
void copyData(const QueryBox& outer, const QueryBox& cacheBox,const QueryBox& intersection, double* data) {
    size_t dim = outer.start.size();
    size_t row = intersection.start[dim - 2];
    size_t col = intersection.start[dim - 1];
    size_t rowSize = intersection.count[dim - 2];
    size_t colSize = intersection.count[dim - 1];
    size_t rowStride = outer.count[dim - 1];
    size_t colStride = 1;
    if (dim > 2) {
        colStride = outer.count[dim - 2] * outer.count[dim - 1];
    }

    size_t nContDim = 1;
    while (nContDim <= dim - 1 &&
           outer.start[dim - nContDim] ==
           intersection.start[dim - nContDim] &&
           outer.count[dim - nContDim] ==
                   intersection.count[dim - nContDim] &&
           outer.count[dim - nContDim] == cacheBox.count[dim - nContDim] &&
           outer.start[dim - nContDim] == cacheBox.start[dim - nContDim])
    {
        ++nContDim;
    }
    // Note: 1 <= nContDim <= dimensions
    size_t nContElems = 1;
    size_t blockSize = 1;
    size_t inOvlpSize = 1;
    for (size_t i = 1; i <= nContDim; ++i)
    {
        nContElems *= (intersection.count[dim - i]);
        blockSize *= (outer.count[dim - i]);
        inOvlpSize *= (cacheBox.count[dim - i]);
    }

    // start_offset is the offset of the first element of the intersection part in the data array
    size_t start_offset = (intersection.start[dim - 2] - outer.start[dim - 2]) * rowStride +
                          (intersection.start[dim - 1] - outer.start[dim - 1]) * colStride;
    size_t inOvlpBase = (intersection.start[dim - 2] - cacheBox.start[dim - 2]) * cacheBox.count[dim - 1] +
                        (intersection.start[dim - 1] - cacheBox.start[dim - 1]) * 1;

    bool run = true;
    // const cacheBox size
    const size_t cacheBoxSize = cacheBox.size();
    while (run){
        // copy data from intersection part to data
        std::memcpy(data + start_offset, intersection.data + inOvlpBase, nContElems * sizeof(double));
        inOvlpBase += inOvlpSize;
        start_offset += blockSize;

        if (inOvlpBase >= cacheBoxSize or start_offset >= outer.size()){
            run = false;
        }
    }
}
