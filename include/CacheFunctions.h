//
// Created by cguo51 on 7/27/23.
//

#ifndef UNITTEST_CACHEFUNCTIONS_H
#define UNITTEST_CACHEFUNCTIONS_H

#include "cachelib/allocator/CacheAllocator.h"
#include "folly/init/Init.h"

namespace facebook {
    namespace cachelib_examples {
        using Cache = cachelib::LruAllocator; // or Lru2QAllocator, or TinyLFUAllocator
        using CacheConfig = typename Cache::Config;
        using CacheKey = typename Cache::Key;
        using CacheReadHandle = typename Cache::ReadHandle;

// Global cache object and a default cache pool
        std::unique_ptr<Cache> gCache_;
        cachelib::PoolId defaultPool_;

        void initializeCache() {
            CacheConfig config;
            config
                    .setCacheSize(1 * 1024 * 1024 * 1024) // 1GB
                    .setCacheName("My Use Case")
                    .setAccessConfig(
                            {25 /* bucket power */, 10 /* lock power */}) // assuming caching 20
                            // million items
                    .validate(); // will throw if bad config
            gCache_ = std::make_unique<Cache>(config);

            // if needed, consider use multiple cache pools, rather than multiple instances
            defaultPool_ =
                    gCache_->addPool("default", gCache_->getCacheMemoryStats().cacheSize);
        }

        void destroyCache() { gCache_.reset(); }

        CacheReadHandle get(CacheKey key) { return gCache_->find(key); }

        bool put(CacheKey key, const std::string& value) {
            auto handle = gCache_->allocate(defaultPool_, key, value.size());
            if (!handle) {
                return false; // cache may fail to evict due to too many pending writes
            }
            std::memcpy(handle->getMemory(), value.data(), value.size());
            gCache_->insertOrReplace(handle);
            return true;
        }
    } // namespace cachelib_examples
} // namespace facebook


#endif //UNITTEST_CACHEFUNCTIONS_H
