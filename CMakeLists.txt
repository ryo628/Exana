# CMake file

# set cmake version
cmake_minimum_required(VERSION 2.8.10.1)
enable_language(C)
enable_language(CXX)
include(dynamorio/make/policies.cmake NO_POLICY_SCOPE)

# set project
project(Exana NONE)

set(DynamoRIO_INTERNAL ON)
set(DynamoRIO_DIR ${PROJECT_BINARY_DIR}/dynamorio/cmake)
# message(${DynamoRIO_DIR})
find_package(DynamoRIO)

# add dynamorio
add_subdirectory(dynamorio)
add_subdirectory(lib)
add_subdirectory(src)
