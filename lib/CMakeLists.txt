# CMake file

project(libtest)

# TODO: need it?
# configure_DynamoRIO_global(OFF ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpermissive")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lstdc++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")

# add src
add_library(wrapper SHARED "wrapper.cpp")
# add_library(memtrace SHARED "memtrace_x86.c;utils.c")
add_library(memtrace SHARED "memtrace_simple.c;utils.c")
add_dependencies(memtrace api_headers)
target_link_libraries(wrapper cachesim)
target_link_libraries(memtrace wrapper)
configure_DynamoRIO_client(memtrace)
# use_DynamoRIO_extension(memtrace "drcontainers")
use_DynamoRIO_extension(memtrace "drmgr")
use_DynamoRIO_extension(memtrace "drreg")
use_DynamoRIO_extension(memtrace "drutil")
use_DynamoRIO_extension(memtrace "drx")
# use_DynamoRIO_extension(memtrace "dr_ir_opnd")
