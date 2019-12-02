# CMake File

cmake_minimum_required(VERSION 2.8)
project(exana)

# add ssource
add_library(cachesim SHARED "CacheSim.cpp;CacheMemory.cpp;MemoryEntry.cpp;FullyAssociativeCacheMemory.cpp;SetAssociativeCacheMemory.cpp")
add_executable(Exana main.cpp)
target_link_libraries(Exana cachesim)
