//
// Created by cguo51 on 7/27/23.
//

#ifndef UNITTEST_QUERYBOX_H
#define UNITTEST_QUERYBOX_H

#include "adios2/common/ADIOSTypes.h"
#include <set>
#include "json.hpp"

class QueryBox {
public:
    adios2::Dims start{};
    adios2::Dims count{};

    // data
    double *data{};

    // size
    size_t size() const {
        size_t s = 1;
        for (auto& d : count) {
            s *= d;
        }
        return s;
    }
};



#endif //UNITTEST_QUERYBOX_H
