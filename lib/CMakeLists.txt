# CMake file

project(libtest)

# TODO: need it?
# configure_DynamoRIO_global(OFF ON)

find_package(DynamoRIO)

# add src
add_library(memtrace SHARED "memtrace_x86.c;utils.c")
configure_DynamoRIO_client(memtrace)
use_DynamoRIO_extension(memtrace "drcontainers")
use_DynamoRIO_extension(memtrace "drmgr")
use_DynamoRIO_extension(memtrace "drreg")
use_DynamoRIO_extension(memtrace "drutil")
use_DynamoRIO_extension(memtrace "drx")

#add_sample_client(memtrace_x86_text   "memtrace_x86.c;utils.c"
#    "drcontainers;drmgr;drreg;drutil;drx")
#configure_DynamoRIO_client(memtrace)
