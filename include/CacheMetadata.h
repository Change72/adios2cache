#include "json.hpp"
#include "QueryBox.h"
#include <fstream>
#include <set>
#include <string>


class CacheMetadata {

    using CacheMetadata = std::unordered_map<std::string, std::set<QueryBox>>;

    

    // Serialize the cache map to a JSON file
    void serializeCacheMetadata(const CacheMetadata& cacheMetadata, const std::string& filename) {
        nlohmann::json jsonData;

        for (const auto& entry : cacheMetadata) {
            nlohmann::json queryBoxesJson;
            for (const auto& queryBox : entry.second) {
                nlohmann::json queryBoxJson;
                queryBoxJson["start"] = queryBox.start;
                queryBoxJson["count"] = queryBox.count;
                queryBoxesJson.push_back(queryBoxJson);
            }

            jsonData[entry.first] = queryBoxesJson;
        }

        std::ofstream file(filename);
        if (file.is_open()) {
            file << jsonData.dump(4); // Indent with 4 spaces for readability
            file.close();
        } else {
            std::cerr << "Failed to open the file for writing." << std::endl;
        }
    }

    // Deserialize the cache map from a JSON file
    CacheMetadata deserializeCacheMetadata(const std::string& filename) {
        CacheMetadata cacheMetadata;
        std::ifstream file(filename);
        if (file.is_open()) {
            nlohmann::json jsonData;
            file >> jsonData;
            file.close();

            for (const auto& entry : jsonData.items()) {
                std::set<QueryBox> queryBoxes;
                for (const auto& queryBoxJson : entry.value()) {
                    QueryBox queryBox;
                    queryBox.start = queryBoxJson["start"].get<adios2::Dims>();
                    queryBox.count = queryBoxJson["count"].get<adios2::Dims>();
                    // Retrieve other members of QueryBox from queryBoxJson as needed
                    queryBoxes.insert(queryBox);
                }

                cacheMetadata[entry.key()] = queryBoxes;
            }
        } else {
            std::cerr << "Failed to open the file for reading." << std::endl;
        }

        return cacheMetadata;
    }
};