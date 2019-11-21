# CMake file

project(libtest)

# TODO: need it?
# configure_DynamoRIO_global(OFF ON)

# add src
add_library(memtrace SHARED "memtrace_x86.c")
add_dependencies(memtrace api_headers)
configure_DynamoRIO_client(memtrace)
use_DynamoRIO_extension(memtrace "drcontainers")
use_DynamoRIO_extension(memtrace "drmgr")
use_DynamoRIO_extension(memtrace "drreg")
use_DynamoRIO_extension(memtrace "drutil")
use_DynamoRIO_extension(memtrace "drx")
