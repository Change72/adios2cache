//
// Created by cguo51 on 7/31/23.
//

#include "cachelib/allocator/CacheAllocator.h"
#include "folly/init/Init.h"
#include <iostream>

using Cache = facebook::cachelib::LruAllocator;
using CacheKey = typename Cache::Key;
using Item = Cache::Item;
using WriteHandle = Cache::WriteHandle;
using ReadHandle = Cache::ReadHandle;
using ChainedAllocs = Cache::ChainedAllocs;
using DestructorData = typename Cache::DestructorData;
using ChainedItemIter = Cache::ChainedItemIter;
using DestructedData = Cache::DestructorData;
using PoolId = int8_t;

class CacheLibInterface {
public:
    // cache instance
    std::unique_ptr<Cache> cache_;
    // pool id for allocation
    PoolId defaultPool_;

    // initialize the cache
    // if persistDir is empty, then we only initialize the in-memory cache
    void initCache(size_t cacheSize, std::string persistDir, unsigned long long nvmSize = 1024 * 1024ULL * 1024ULL,
                   uint64_t deviceMetadataSize = 16 * 1024 * 1024, uint64_t regionSize = 1 * 1024 * 1024,
                   uint64_t bucketSize = 4096, uint64_t bucketBfSize = 8) {
        // the config for the allocator that also includes the nvm config
        Cache::Config allocConfig_;
        // Set the in-memory cache size
        allocConfig_.setCacheSize(cacheSize);

        if (!persistDir.empty()) {
            // the config for the nvm cache
            facebook::cachelib::navy::NavyConfig navyConfig;
            // the directory of SSD part in hybrid cache
            std::string cacheDir_;
            // cacheDir_ = folly::sformat("/tmp/nvmcache-cachedir/gc_nvmcache_test");
            cacheDir_ = folly::sformat(persistDir);
            facebook::cachelib::util::makeDir(cacheDir_);

            // set the navy config, with 1GB SSD cache
            navyConfig.setSimpleFile(cacheDir_ + "/navy", nvmSize);
            navyConfig.setDeviceMetadataSize(deviceMetadataSize);
            // navyConfig.setBlockSize(1024);
            navyConfig.setNavyReqOrderingShards(10);

            // Region is 1MB
            navyConfig.blockCache().setRegionSize(regionSize);

            // Bucket size: 4096 should be aligned to ioAlignSize: 4096
            navyConfig.bigHash()
                    .setSizePctAndMaxItemSize(50, 100)
                    .setBucketSize(bucketSize)
                    .setBucketBfSize(bucketBfSize);

            allocConfig_.enableCachePersistence(cacheDir_);

            facebook::cachelib::LruAllocator::NvmCacheConfig nvmConfig;
            nvmConfig.navyConfig = navyConfig;
            // nvmConfig.enableFastNegativeLookups = false;
            allocConfig_.enableNvmCache(nvmConfig);
        }

        bool attached = false;
        // Try to attach to an existing cache from memory
        // CacheLib cache persistent:
        // 1. Memory: cache_->enableCachePersistence(cacheDir_);
        // which is retained in the memory and would be lost after the restart.
        // Cache::SharedMemAttach is trying to attach to the cache in the memory.
        // 2. Disk: Navy cache, which is persistent in the disk.
        try {
            cache_ = std::make_unique<Cache>(Cache::SharedMemAttach, allocConfig_);
            // Cache is now restored
            attached = true;
            std::cout << "attached = true" << std::endl;
        }
        catch (const std::exception &ex) {
            // Attaching failed. Create a new one but make sure that
            // the old cache is destroyed before creating a new one.
            // This allows us to release any held resources (such as
            // open file descriptors and associated fcntl locks).
            cache_.reset();
            std::cerr << "Couldn't attach to cache: " << ex.what() << std::endl;
            cache_ = std::make_unique<Cache>(Cache::SharedMemNew, allocConfig_);
        }

        if (!attached) {
            // Add pool only if the cache is not restored above
            defaultPool_ = cache_->addPool("default", cache_->getCacheMemoryStats().ramCacheSize);
        }
    }

    ReadHandle get(CacheKey key) {
        auto handle = cache_->find(key);
        if (!handle) {
            std::cout << key << "no exist cache" << std::endl;
            return nullptr;
        }

        if (handle && handle.isReady()) {
            std::string value(reinterpret_cast<const char *>(handle->getMemory()), handle->getSize());
            std::cout << "value: " << value << std::endl;
        } else {
            std::cout << "handle is not ready" << std::endl;
        }
        return handle;
    }

    bool read_and_verify(CacheKey key, const std::string &value) {
        auto handle = cache_->find(key);
        //        auto semiFuture = handle.toSemiFuture();
        //
        //        /* Attach (optional) callback that is invoked when semifuture is executed */
        //        auto sf = std::move(semiFuture).deferValue([] (const auto itemHandle) {
        //            /* Do something with the item */
        //        });
        //
        //        /* Schedule semi future to be executed async, when the item is ready */
        //        std::move(semiFuture).via(
        //                folly::Executor::getKeepAliveToken(
        //                        folly::EventBaseManager::get()->getExistingEventBase()));
        if (!handle) {
            std::cout << key << "no exist cache" << std::endl;
            return false;
        }

        if (handle && handle.isReady()) {
            std::string value_read(reinterpret_cast<const char *>(handle->getMemory()), handle->getSize());
            // check if the value is correct
            if (value_read != value) {
                std::cout << "value is not correct" << std::endl;
            } else {
                std::cout << "successful" << std::endl;
            }
        } else {
            std::cout << "handle is not ready" << std::endl;
        }
        return true;
    }

    bool put(CacheKey key, const std::string &value) {
        auto handle = cache_->allocate(defaultPool_, key, value.size());
        if (!handle) {
            return false; // cache may fail to evict due to too many pending writes
        }
        std::memcpy(handle->getMemory(), value.data(), value.size());
        cache_->insertOrReplace(handle);
        return true;
    }

    // destroy the cache
    void destroyCache() {
        auto res = cache_->shutDown();
        if (res == Cache::ShutDownStatus::kSuccess) {
            // Successfully shut down the cache. Can attempt recovering.
            std::cout << "Successfully shut down the cache. Can attempt recovering." << std::endl;
        } else {
            // Failure.
            std::cout << "Failure." << std::endl;
        }
    }
};