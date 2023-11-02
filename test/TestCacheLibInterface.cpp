#include "CachelibInterface.h"
#include <iostream>

// sample usage: ./CacheTest -write=true -read=true -read_inverse=false -operation_num=100000 > ../log/testCacheLib.log

int main(int argc, char **argv)
{
    size_t cache_size = 1024 * 1024 * 1024;
    std::string persist_dir = "/tmp/nvmcache-cachedir/gc_nvmcache_test";
    CacheLibInterface cacheLibInterface;
    cacheLibInterface.initCache(cache_size, persist_dir);

    std::map<std::string, std::string> parameters;

    // set default parameters
    parameters["write"] = "true";
    parameters["read"] = "true";
    parameters["read_inverse"] = "false";
    parameters["operation_num"] = "100000";

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        size_t pos = arg.find('=');

        if (pos != std::string::npos)
        {
            std::string key = arg.substr(1, pos - 1); // Remove the leading '-'
            std::string value = arg.substr(pos + 1);
            parameters[key] = value;
        }
    }

    std::string value(16 * 1024, 'a');
    int op_num = std::stoi(parameters["operation_num"]);
    if (parameters["write"] == "true")
    {
        std::cout << "start to put keys" << std::endl;
        for (int i = 0; i < op_num; i++)
        {
            std::string key = "key" + std::to_string(i);
            if (!cacheLibInterface.put(key, value))
            {
                std::cout << key << " put failed" << std::endl;
            }
        }
    }

    if (parameters["read"] == "true")
    {
        std::cout << "start to read keys" << std::endl;
        std::string key = "";
        for (int i = 0; i < op_num; i++)
        {
            if (parameters["read_inverse"] == "true")
            {
                key = "key" + std::to_string(op_num - i);
            }
            else
            {
                key = "key" + std::to_string(i);
            }
            if (!cacheLibInterface.read_and_verify(key, value))
            {
                std::cout << key << " read failed" << std::endl;
            }
        }
    }

    cacheLibInterface.destroyCache();
    return 0;
}