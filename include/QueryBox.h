//
// Created by cguo51 on 7/27/23.
//

#ifndef UNITTEST_QUERYBOX_H
#define UNITTEST_QUERYBOX_H

#include "adios2/common/ADIOSTypes.h"
#include <set>
#include "json.hpp"

// QueryBox is a class to represent a query box in a multi-dimensional space
class QueryBox
{
public:
    adios2::Dims start{};
    adios2::Dims count{};

    // data
    double *data{};

    // size
    size_t size() const
    {
        size_t s = 1;
        for (auto &d : count)
        {
            s *= d;
        }
        return s;
    }

    // Serialize QueryBox to a JSON string
    std::string serializeQueryBox(const QueryBox &box)
    {
        nlohmann::json jsonBox;
        jsonBox["start"] = box.start;
        jsonBox["count"] = box.count;
        return jsonBox.dump();
    }

    // Deserialize JSON string to a QueryBox
    QueryBox deserializeQueryBox(const std::string &jsonString)
    {
        nlohmann::json jsonBox = nlohmann::json::parse(jsonString);
        QueryBox box;
        box.start = jsonBox["start"].get<adios2::Dims>();
        box.count = jsonBox["count"].get<adios2::Dims>();
        return box;
    }

    // determine if a query box is interacted in another query box, return intersection part as a new query box
    bool isInteracted(const QueryBox& box, QueryBox& intersection){
        if (start.size() != box.start.size() || start.size() != count.size() || start.size() != box.count.size())
        {
            return false;
        }
        for (size_t i = 0; i < start.size(); ++i)
        {
            if (start[i] > box.start[i] + box.count[i] || box.start[i] > start[i] + count[i])
            {
                return false;
            }
        }
        for (size_t i = 0; i < start.size(); ++i)
        {
            intersection.start[i] = std::max(start[i], box.start[i]);
            intersection.count[i] = std::min(start[i] + count[i], box.start[i] + box.count[i]) - intersection.start[i];
        }
        return true;
    }

    // if a query box is interacted with one of previous query boxes, return the remaining part as a set of query boxes
    // intersection is inside outer
    std::set<QueryBox> getRemaining2D(const QueryBox& outer, const QueryBox& intersection) {
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
                
            }
        }
        return remaining;
    }

};

#endif // UNITTEST_QUERYBOX_H
