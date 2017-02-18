set(jpp_util_sources mmap.cpp memory.cpp logging.cpp string_piece.cc status.cpp)
set(jpp_util_headers mmap.hpp status.hpp fixed_queue.hpp memory.hpp characters.hpp types.hpp logging.hpp common.hpp string_piece.h)
set(jpp_util_test_srcs memory_test.cpp mmap_test.cc string_piece_test.cc)

add_library(jpp_util ${jpp_util_sources} ${jpp_util_headers})
jpp_test_executable(jpp_util_test ${jpp_util_test_srcs} ${jpp_util_headers})
target_link_libraries(jpp_util_test jpp_util)