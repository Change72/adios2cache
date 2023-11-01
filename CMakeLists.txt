cmake_minimum_required(VERSION 3.12)
project(UnitTest)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)

include_directories(include)
file(GLOB CPP_SOURCES src/*.cpp)

# Set the CLBASE variable
set(CLBASE "/data/gc/ornl/CacheLib/")

set(ADIOS_DIR "/data/gc/rocksdb-index/ADIOS2/")
include_directories(${ADIOS_DIR}/include)
set(ADIOS2_LIBRARY ${ADIOS_DIR}/adios2-build/lib/libadios2_cxx11.so)

# Set the CLCMAKE variable
set(CLCMAKE "${CLBASE}/cachelib/cmake")

# Set the PREFIX variable
set(PREFIX "${CLBASE}/opt/cachelib/")

# Set CMake parameters
set(CMAKE_PARAMS "-DCMAKE_INSTALL_PREFIX='${PREFIX}' -DCMAKE_MODULE_PATH='${CLCMAKE}'")

# Update CMAKE_PREFIX_PATH
list(APPEND CMAKE_PREFIX_PATH "${CLCMAKE}" "${PREFIX}/lib/cmake" "${PREFIX}/lib64/cmake" "${PREFIX}/lib" "${PREFIX}/lib64" "${PREFIX}")

# FindSodium.cmake
list(APPEND CMAKE_MODULE_PATH "${CLCMAKE}")

# Update PKG_CONFIG_PATH
set(ENV{PKG_CONFIG_PATH} "${PREFIX}/lib/pkgconfig:${PREFIX}/lib64/pkgconfig:$ENV{PKG_CONFIG_PATH}")

# Update LD_LIBRARY_PATH
set(ENV{LD_LIBRARY_PATH} "${PREFIX}/lib:${PREFIX}/lib64:$ENV{LD_LIBRARY_PATH}")

# Find cachelib
find_package(cachelib CONFIG REQUIRED)

# Add your project source files
add_executable(UnitTest main.cpp ${CPP_SOURCES})

# Link cachelib to your project
target_link_libraries(UnitTest  cachelib ${ADIOS2_LIBRARY})

# Set include directories for cachelib
target_include_directories(UnitTest PRIVATE ${cachelib_INCLUDE_DIRS})


# Add your project source files
add_executable(CacheTest test/TestCacheLibWithNavy.cpp ${CPP_SOURCES})

# Link cachelib to your project
target_link_libraries(CacheTest  cachelib )

# Set include directories for cachelib
#message(STATUS "cachelib_INCLUDE_DIRS: ${cachelib_INCLUDE_DIRS}")
target_include_directories(CacheTest PRIVATE ${cachelib_INCLUDE_DIRS})

add_executable(TestNDPointsCluster test/TestNDPointsCluster.cpp ${CPP_SOURCES})