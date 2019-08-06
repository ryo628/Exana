# CMake file

project(libtest)

# TODO: need it?
# configure_DynamoRIO_global(OFF ON)

find_package(DynamoRIO)
if(NOT DynamoRIO_FOUND)
  message(FATAL_ERROR"DynamoRIO package required to build")
endif(NOT DynamoRIO_FOUND)

# add src
add_library(memtrace SHARED "memtrace_x86.c;utils.c")
configure_DynamoRIO_client(memtrace)
use_DynamoRIO_extension(memtrace "drcontainers")
use_DynamoRIO_extension(memtrace "drmgr")
use_DynamoRIO_extension(memtrace "drreg")
use_DynamoRIO_extension(memtrace "drutil")
use_DynamoRIO_extension(memtrace "drx")
