#include <iostream>
#include <vector>
#include <cmath>
#include "adios2/common/ADIOSTypes.h"
#include "QueryBox.h"

class CacheBitmap
{
public:
    // start
    adios2::Dims start{};
    // count
    adios2::Dims count{};
    // gap
    adios2::Dims gap{};

    // data dimension
    adios2::Dims dataDims{};

    // data
    std::vector<bool> data{};

    // initialize the cache bitmap
    void initCacheBitmap(const adios2::Dims& start, const adios2::Dims& count, const adios2::Dims& gap) {
        this->start = start;
        this->count = count;
        this->gap = gap;
        size_t size = 1;
        // size *= roof(count[0] / gap[0]);

        for (size_t i = 0; i < count.size(); ++i) {
            dataDims.push_back(std::ceil(count[i] / gap[i]));
            size *= dataDims[i];
        }
        data = std::vector<bool>(size, false);
    }

    // size
    size_t size() const
    {
        return data.size();
    }

    // bounds is a n dimensional vector, each element is a pair of int
    void updateCacheBitmapByBounds(const std::vector<std::pair<int, int>>& bounds, int dimension, int offset) {
        if (dimension == bounds.size()) {
            // If we've filled values for all dimensions, add the point to the results
            self.data[offset] = true;
            return;
        }

        int lowerBound = bounds[dimension].first;
        int upperBound = bounds[dimension].second;

        // Iterate over all possible values within the current dimension's range
        for (int i = lowerBound; i < upperBound; i++) {
            offset += self.dataDims[dimension] * i;
            updateCacheBitmapByBounds(bounds, dimension + 1, offset);
        }
    }

    // update the cache bitmap
    void updateCacheBitmap(const QueryBox& box) {
        // if dimension is not equal, return
        if (start.size() != box.start.size() || start.size() != count.size() || start.size() != box.count.size())
        {
            return;
        }
        // if not interacted, return
        for (size_t i = 0; i < start.size(); ++i)
        {
            if (start[i] > box.start[i] + box.count[i] || box.start[i] > start[i] + count[i])
            {
                return;
            }
        }
        // indices can n dimensional
        std::vector<std::pair<int, int>> indices;

        // if interacted, update the cache bitmap
        for (size_t i = 0; i < start.size(); ++i)
        {
            size_t start = std::max(this->start[i], box.start[i]);
            size_t end = std::min(this->start[i] + this->count[i], box.start[i] + box.count[i]);
            size_t startIdx = (start - this->start[i] - 1) / gap[i] + 1;
            size_t endIdx = (end - this->start[i]) / gap[i];
            if (startIdx >= endIdx)
            {
                // there is no entire cache block in the query box
                return;
            }
            indices.push_back(std::pair<int, int>{startIdx, endIdx});
        }
        updateCacheBitmapByBounds(indices, 0, 0);
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

