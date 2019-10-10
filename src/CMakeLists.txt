# CMake File

cmake_minimum_required(VERSION 2.8)
project(exana)

#set(CMAKE_C_FLAGS "-pg")
#set(CMAKE_CXX_FLAGS "-pg")

# add ssource
add_executable(Exana main.cpp CacheSim.cpp CacheMemory.cpp MemoryEntry.cpp FullyAssociativeCacheMemory.cpp SetAssociativeCacheMemory.cpp )
