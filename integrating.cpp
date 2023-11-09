#include "adios2.h"

#include "include/QueryBox.h"
#include "include/CacheLibInterface.h"
#include "include/utils.h"
#include "include/CacheMetadata.h"
#include "include/NdPointCluster.h"
#include "include/CacheBitmap.h"




int main(int argc, char** argv) {
    // initialize the cache
    size_t cache_size = 1024 * 1024 * 1024;
    std::string persist_dir = "/tmp/nvmcache-cachedir/gc_nvmcache_test";
    CacheLibInterface cacheLibInterface;
    cacheLibInterface.initCache(cache_size, persist_dir);

    // initialize the adios2 engine
    std::string filename = "/data/gc/rocksdb-index/ADIOS2/adios2-build/output/heat.bp5";
    adios2::ADIOS adios;
    adios2::IO bpIO = adios.DeclareIO("ReadBP");
    adios2::Engine bpReader = bpIO.Open(filename, adios2::Mode::Read);
    adios2::StepStatus status = bpReader.BeginStep(adios2::StepMode::Read);
    std::string inquireVariableName = "T";

    bpIO.SetEngine("BP5");
    bool firstStep = false;
    int step = 0;

    // initialize the in-memory cache map
    std::string cacheMetadataFilename = "./data/cacheMetadata.json";
    CacheMetadata cacheMetadata;
    if (std::filesystem::exists(cacheMetadataFilename)) {
        cacheMetadata = deserializeCacheMap(cacheMetadataFilename);
    }

    // init a list of query box by setting start and count
    std::vector<QueryBox> queryBoxList;
    QueryBox queryBox1;
    queryBox1.start = {100, 150};
    queryBox1.count = {200, 100};
    queryBoxList.push_back(queryBox1);

    QueryBox queryBox2;
    queryBox2.start = {300, 330};
    queryBox2.count = {200, 100};
    queryBoxList.push_back(queryBox2);

    QueryBox queryBox3;
    queryBox3.start = {200, 240};
    queryBox3.count = {200, 100};
    queryBoxList.push_back(queryBox3);

    for (auto& queryBox : queryBoxList) {
        step++;
        std::cout << "current query num: " << step << std::endl;
        std::cout << "current query box: " << serializeQueryBox(queryBox) << std::endl;
        if (firstStep) {
            // Promise that we are not going to change the variable sizes
            // nor add new variables
            bpReader.LockReaderSelections();

        }
        adios2::Variable<double> variableMetadata =
                bpIO.InquireVariable<double>(inquireVariableName);

        std::vector<double> result;
        result.reserve(queryBox.size());

        // check cache
        std::string queryKey = filename + inquireVariableName + std::to_string(0);
        // std::string queryKey = filename + inquireVariableName + std::to_string(step);

        // if queryKey is in cacheMetadata, then check cache
        if (cacheMetadata.find(queryKey) != cacheMetadata.end()) {
            // initial multiple threads to fetch data from cache



            // if not in cache, update cacheMetadata

        } 

        // based on updated cacheMetadata, generate read strategy by using k-means clustering
        // evaluate the read strategy by using cost function
        NdPointCluster ndPointCluster;
        


        // read data from adios2 engine

        // integrate all data from cache and adios2 engine as the final result

        // update cache and cacheMetadata

        // verify correctness of the result with the direct result from adios2 engine

        // end step


    }

    // destroy the cache
    cacheLibInterface.destroyCache();
    // serialize the cache map to a JSON file
    serializeCacheMetadata(cacheMetadata, cacheMetadataFilename);
}
