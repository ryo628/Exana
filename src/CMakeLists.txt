# CMake File

cmake_minimum_required(VERSION 2.8)
project(exana)

# add ssource
add_executable(Exana main.cpp CacheSim.cpp Utils.cpp CacheMemory.cpp MemoryEntry.cpp FullyAssociativeCacheMemory.cpp SetAssociativeCacheMemory.cpp )
