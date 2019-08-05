# CMake file

project(libtest)

# TODO: need it?
# configure_DynamoRIO_global(OFF ON)

# add src
add_library(memtrace "memtrace_x86.c;utils.c")
configure_DynamoRIO_client(memtrace)
use_DynamoRIO_extension("drcontainers;")
use_DynamoRIO_extension("drmgr;")
use_DynamoRIO_extension("drreg;")
use_DynamoRIO_extension("drutil;")
use_DynamoRIO_extension("drx;")

#add_sample_client(memtrace_x86_text   "memtrace_x86.c;utils.c"
#    "drcontainers;drmgr;drreg;drutil;drx")
#configure_DynamoRIO_client(memtrace)
