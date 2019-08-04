# CMake file

# TODO: need it?
# configure_DynamoRIO_global(OFF ON)

# add src
add_sample_client(memtrace_x86_text   "memtrace_x86.c;utils.c"
    "drcontainers;drmgr;drreg;drutil;drx")
configure_DynamoRIO_client(memtrace_x86_text)
