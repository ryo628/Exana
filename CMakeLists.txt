# CMake file

# set cmake version
cmake_minimum_required(VERSION 2.8.10.1)
enable_language(C)
enable_language(CXX)
include(dynamorio/make/policies.cmake NO_POLICY_SCOPE)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# set cmake flag
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
#set(CMAKE_C_FLAGS "-pg")
#set(CMAKE_CXX_FLAGS "-pg")

# set project
project(Exana NONE)

# add dynamorio
set(DynamoRIO_INTERNAL ON)
set(DynamoRIO_DIR ${PROJECT_BINARY_DIR}/dynamorio/cmake)
add_subdirectory(dynamorio)
find_package(DynamoRIO REQUIRED)

add_subdirectory(src)
add_subdirectory(lib)
