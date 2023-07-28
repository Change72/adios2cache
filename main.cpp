/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cachelib/allocator/CacheAllocator.h"
#include "folly/init/Init.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2.h"
#include <set>
#include <QueryBox.h>
#include <CacheFunctions.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <cstring>
#include <iostream>
#include "adios2/helper/adiosFunctions.h"


using namespace facebook::cachelib_examples;


// Serialize QueryBox to a JSON string
std::string serializeQueryBox(const QueryBox& box) {
    nlohmann::json jsonBox;
    jsonBox["start"] = box.start;
    jsonBox["count"] = box.count;
    return jsonBox.dump();
}

// Deserialize JSON string to a QueryBox
QueryBox deserializeQueryBox(const std::string& jsonString) {
    nlohmann::json jsonBox = nlohmann::json::parse(jsonString);
    QueryBox box;
    box.start = jsonBox["start"].get<adios2::Dims>();
    box.count = jsonBox["count"].get<adios2::Dims>();
    return box;
}


// Custom comparison operator for QueryBox
bool operator<(const QueryBox& lhs, const QueryBox& rhs) {
    return lhs.start < rhs.start || (lhs.start == rhs.start && lhs.count < rhs.count);
}

// Custom comparison operator for std::set<QueryBox>
bool operator<(const std::set<QueryBox>& lhs, const std::set<QueryBox>& rhs) {
    return lhs.size() < rhs.size() ||
           (lhs.size() == rhs.size() && std::lexicographical_compare(
                   lhs.begin(), lhs.end(), rhs.begin(), rhs.end()));
}

// Custom comparison operator for std::unordered_map<std::string, std::set<QueryBox>>
bool operator<(const std::unordered_map<std::string, std::set<QueryBox>>& lhs,
               const std::unordered_map<std::string, std::set<QueryBox>>& rhs) {
    return lhs.size() < rhs.size() ||
           (lhs.size() == rhs.size() && std::lexicographical_compare(
                   lhs.begin(), lhs.end(), rhs.begin(), rhs.end()));
}


// determine if a query box is contained in another query box
bool isContained(const QueryBox& outer, const QueryBox& inner) {
    for (size_t i = 0; i < outer.start.size(); i++) {
        if (outer.start[i] > inner.start[i] ||
            outer.start[i] + outer.count[i] < inner.start[i] + inner.count[i]) {
            return false;
        }
    }
    return true;
}

// determine if a query box is interacted in another query box, return intersection part as a new query box
bool isInteracted(const QueryBox& outer, const QueryBox& inner, QueryBox& intersection) {
    for (size_t i = 0; i < outer.start.size(); i++) {
        if (outer.start[i] > inner.start[i] + inner.count[i] - 1 ||
            outer.start[i] + outer.count[i] - 1  < inner.start[i]) {
            return false;
        }
        intersection.start.push_back(std::max(outer.start[i], inner.start[i]));
        intersection.count.push_back(std::min(outer.start[i] + outer.count[i],
                                              inner.start[i] + inner.count[i]) -
                                     intersection.start[i]);
    }
    return true;
}


// def a function to remove the intersection part from the outer box
QueryBox doExclude2d(const QueryBox& outer, const QueryBox& intersection, bool isRowMajor, bool isExcludeHorizontal){
    QueryBox remaining;
    remaining.start = outer.start;
    remaining.count = outer.count;
    if (isRowMajor && isExcludeHorizontal) {
        remaining.count[0] = intersection.start[0] - outer.start[0];
        remaining.count[1] = outer.count[1];
        if (outer.start[0] == intersection.start[0]) {
            remaining.start[0] = intersection.start[0] + intersection.count[0];
            remaining.count[0] = outer.start[0] + outer.count[0] - remaining.start[0];
        }
    }
    return remaining;
}


// def a function to remove the intersection part from the outer box
QueryBox doExclude3d(const QueryBox& outer, const QueryBox& intersection){
    QueryBox remaining;
    remaining.start = outer.start;
    remaining.count = outer.count;
    remaining.count[0] = intersection.start[0] - outer.start[0];
    remaining.count[1] = intersection.start[1] - outer.start[1];
    remaining.count[2] = outer.count[2];
    if (outer.start[0] == intersection.start[0]) {
        remaining.start[0] = intersection.start[0] + intersection.count[0];
        remaining.count[0] = outer.start[0] + outer.count[0] - remaining.start[0];
    }
    if (outer.start[1] == intersection.start[1]) {
        remaining.start[1] = intersection.start[1] + intersection.count[1];
        remaining.count[1] = outer.start[1] + outer.count[1] - remaining.start[1];
    }
    return remaining;
}

// if a query box is interacted with another query box, return the remaining part as a set of query boxes
// intersection is inside outer
std::set<QueryBox> getRemaining(const QueryBox& outer, const QueryBox& intersection) {
    std::set<QueryBox> remaining;
    bool isRowMajor = true;
    // copy outer box
    QueryBox outerCopy;
    for (size_t i = 0; i < outer.start.size(); i++) {
        outerCopy.start.push_back(outer.start[i]);
        outerCopy.count.push_back(outer.count[i]);
    }

    if (isRowMajor){
        bool cutting = true;
        while(cutting) {
            // if fully matched, no remaining
            if (outerCopy.start == intersection.start && outerCopy.count == intersection.count) {
                cutting = false;
                break;
            }

            QueryBox box;
            box.start = outerCopy.start;
            box.count = outerCopy.count;
            // if not fully matched, check each dimension
            // first, cut from tail of the first dimension
            if (outerCopy.start[0] == intersection.start[0] and outerCopy.count[0] != intersection.count[0]) {
                box.start[0] = intersection.start[0] + intersection.count[0];
                box.count[0] = outerCopy.count[0] - intersection.count[0];
                remaining.insert(box);

                outerCopy.count[0] = intersection.count[0];
                continue;
            }

            // second, cut from head of the first dimension
            if (outerCopy.start[0] != intersection.start[0]){
                box.count[0] = intersection.start[0] - outerCopy.start[0];
                remaining.insert(box);
                outerCopy.count[0] = outerCopy.start[0] + outerCopy.count[0] - intersection.start[0];
                outerCopy.start[0] = intersection.start[0];
                continue;
            }

//            // second, cut from head of the first dimension
//            if (outerCopy.start[0] + outerCopy.count[0] == intersection.start[0] + intersection.count[0] and outerCopy.count[0] != intersection.count[0]){
//                box.count[0] = intersection.start[0] - outerCopy.start[0];
//                remaining.insert(box);
//                outerCopy.start[0] = intersection.start[0];
//                outerCopy.count[0] = intersection.count[0];
//                continue;
//            }


            // third, cut from tail of the second dimension
            if (outerCopy.start[1] == intersection.start[1] and outerCopy.count[1] != intersection.count[1]) {
                box.start[1] = intersection.start[1] + intersection.count[1];
                box.count[1] = outerCopy.count[1] - intersection.count[1];
                remaining.insert(box);

                outerCopy.count[1] = intersection.count[1];
                continue;
            }

            // fourth, cut from head of the second dimension
            if (outerCopy.start[1] != intersection.start[1]){
                box.count[1] = intersection.start[1] - outerCopy.start[1];
                remaining.insert(box);
                outerCopy.count[1] = outerCopy.start[1] + outerCopy.count[1] - intersection.start[1];
                outerCopy.start[1] = intersection.start[1];
                continue;
            }

//            // fourth, cut from head of the second dimension
//            if (outerCopy.start[1] + outerCopy.count[1] == intersection.start[1] + intersection.count[1]){
//                box.count[1] = intersection.start[1] - outerCopy.start[1];
//                remaining.insert(box);
//                outerCopy.start[1] = intersection.start[1];
//                outerCopy.count[1] = intersection.count[1];
//                continue;
//            }
        }
    }
    return remaining;
}


// if a query box is interacted with another query box, copy the data from the intersection part
// query box is 2d or 3d array, data is stored in row major
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

int main(int argc, char** argv) {
    folly::init(&argc, &argv);

    initializeCache();

    std::unordered_map<std::string, std::set<QueryBox>> cacheMap;

    std::string filename = "/data/gc/rocksdb-index/ADIOS2/adios2-build/output/heat.bp5";

    /** ADIOS class factory of IO class objects */
    adios2::ADIOS adios;

    /*** IO class object: settings and factory of Settings: Variables,
     * Parameters, Transports, and Execution: Engines */
    adios2::IO bpIO = adios.DeclareIO("ReadBP");

    /** Engine derived class, spawned to start IO operations */
    adios2::Engine bpReader = bpIO.Open(filename, adios2::Mode::Read);

    adios2::StepStatus status =
            bpReader.BeginStep(adios2::StepMode::Read);

    bpIO.SetEngine("BP5");
    bool firstStep = false;
    int step = 0;

    // init a list of query box by setting start and count
    std::vector<QueryBox> queryBoxList;
    QueryBox queryBox1;
    queryBox1.start = {100, 150};
    queryBox1.count = {200, 100};
    queryBoxList.push_back(queryBox1);

    QueryBox queryBox3;
    queryBox3.start = {300, 330};
    queryBox3.count = {200, 100};
    queryBoxList.push_back(queryBox3);

    QueryBox queryBox2;
    queryBox2.start = {200, 240};
    queryBox2.count = {200, 100};
    queryBoxList.push_back(queryBox2);

    QueryBox queryBox4;
    queryBox4.start = {500, 520};
    queryBox4.count = {200, 100};
    queryBoxList.push_back(queryBox4);

    QueryBox queryBox5;
    queryBox5.start = {0, 200};
    queryBox5.count = {500, 1};
    queryBoxList.push_back(queryBox5);

    QueryBox queryBox6;
    queryBox6.start = {490, 490};
    queryBox6.count = {400, 400};
    queryBoxList.push_back(queryBox6);


    for (auto& queryBox : queryBoxList) {
        // print current for loop number
        step++;
        std::cout << "current query num: " << step << std::endl;


        if (firstStep) {
            // Promise that we are not going to change the variable sizes
            // nor add new variables
            bpReader.LockReaderSelections();

        }

        std::string inquireVariableName = "T";

        adios2::Variable<double> x_100_position =
                bpIO.InquireVariable<double>(inquireVariableName);

        // Create a 2D selection for the subset


        std::vector<double> myDouble;
        myDouble.resize(queryBox.size());
        myDouble.reserve(queryBox.size());

        // check cache
        std::string queryTypeKey = filename + inquireVariableName + std::to_string(0);
//        std::string queryTypeKey = filename + inquireVariableName + std::to_string(step);
        // remaining part of query box
        std::set<QueryBox> remaining = {queryBox};
        // if queryTypeKey in cacheMap
        if (cacheMap.find(queryTypeKey) != cacheMap.end()) {
            for (auto &box: cacheMap[queryTypeKey]) {
                std::set<QueryBox> remainingNew;
                // get remaining part of query box
                for (auto &remaining_box: remaining) {
                    QueryBox intersection;
                    if (isInteracted(box, remaining_box, intersection)) {
                        // CacheLib hit
                        std::string queryKey = queryTypeKey + "_" + serializeQueryBox(box);

                        auto handle = get(queryKey);
                        if (handle) {
                            // CacheLib hit
                            std::string value(reinterpret_cast<const char *>(handle->getMemory()), handle->getSize());
                            // intersection.data reserve size
                            intersection.data = new double[box.size()];
                            std::cout << box.size() << std::endl;
//                        std::cout << value.size() << std::endl;
                            std::memcpy(intersection.data, value.data(), box.size() * sizeof(double));

                            // copy to the final result
                            copyData(queryBox, box, intersection, myDouble.data());
                            std::cout << value.size() << std::endl;
                        }

                        // update remaining part, union of remainingNew and remaining
                        for (auto &box1: getRemaining(remaining_box, intersection)) {
                            remainingNew.insert(box1);
                        }

                    } else {
                        // CacheLib miss
                        // update remaining part, union of remainingNew and remaining
                        remainingNew.insert(remaining_box);
                    }

                }
                remaining = remainingNew;
            }

        }
        // print remaining
        for (auto &box1: remaining) {
            std::cout << "remaining: " << serializeQueryBox(box1) << std::endl;
        }

        std::vector<double> selectResult;
        // remaining part of query box
        for (auto &remaining_box: remaining) {
            // fetch result

            selectResult.clear();
            std::cout << remaining_box.size() << std::endl;
            selectResult.resize(std::max(remaining_box.size(), selectResult.size()));
        }

        for (auto &remaining_box: remaining) {
            // print current step
//            std::cout << bpReader.CurrentStep() << std::endl;

            x_100_position.SetSelection(
                    adios2::Box<adios2::Dims>(remaining_box.start, remaining_box.count));


            // Arrays are read by scheduling one or more of them
            // and performing the reads at once
            bpReader.Get<double>(x_100_position, selectResult, adios2::Mode::Sync);
            /*printDataStep(Tin.data(), settings.readsize.data(),
              settings.offset.data(), rank, step); */

//            bpReader.EndStep();
            QueryBox remaining_box_new = remaining_box;
            remaining_box_new.data = selectResult.data();

            // copy to the final result
            QueryBox cacheBox;
            cacheBox.start = {0, 0};
            cacheBox.count = {0, 0};
            copyData(queryBox, remaining_box_new, remaining_box_new, myDouble.data());
//            std::cout << bpReader.CurrentStep() << std::endl;
        }
        selectResult.clear();
        // insert queryBox into cacheMap
        cacheMap[queryTypeKey].emplace(queryBox);

        std::cout << myDouble.size() << std::endl;
        std::cout << queryBox.size() << std::endl;



        // CacheLib insert this query result
        std::string queryKey = queryTypeKey + "_" + serializeQueryBox(queryBox);
        auto res = put(queryKey,
                       std::string(reinterpret_cast<const char *>(myDouble.data()), queryBox.size() * sizeof(double)));

        std::ignore = res;
        assert(res);

        // direct read query box, to finish data correctness check
//        std::cout << bpReader.CurrentStep() << std::endl;
//        std::cout << queryBox.size() << std::endl;
        std::vector<double> directSelectResult;
        x_100_position.SetSelection(
                adios2::Box<adios2::Dims>(queryBox.start, queryBox.count));
        bpReader.Get<double>(x_100_position, directSelectResult, adios2::Mode::Sync);

        // check correctness


        // check correctness
        for (int i = 0; i < queryBox.size(); i++) {
            if (myDouble[i] != directSelectResult[i]) {
                std::cout << "error at element:" << std::to_string(i) << std::endl;
            }
        }
        myDouble.clear();




    }
    bpReader.EndStep();
    destroyCache();
}
