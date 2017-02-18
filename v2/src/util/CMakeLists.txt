set(jpp_util_sources mmap.cpp memory.cpp logging.cpp)
set(jpp_util_headers mmap.hpp status.hpp fixed_queue.hpp memory.hpp characters.hpp types.hpp logging.hpp common.hpp status.cpp)


add_library(jpp_util ${jpp_util_sources} ${jpp_util_headers})

jpp_test(memory_test.cpp jpp_util)