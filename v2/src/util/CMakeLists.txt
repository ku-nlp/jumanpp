set(jpp_util_sources mmap.cc memory.cpp logging.cpp string_piece.cc status.cpp
        csv_reader.cc coded_io.cc characters.cc soa.cc)
set(jpp_util_headers mmap.h status.hpp fixed_queue.hpp memory.hpp characters.h types.hpp logging.hpp common.hpp
        string_piece.h csv_reader.h coded_io.h flatrep.h flatset.h flatmap.h murmur_hash.h array_slice_internal.h
        array_slice.h inlined_vector.h soa.h stl_util.h hashing.h char_buffer.h serialization.h serialization_test.cc)
set(jpp_util_test_srcs memory_test.cpp mmap_test.cc string_piece_test.cc
        csv_reader_test.cc coded_io_test.cc characters_test.cpp hashing_test.cc
        array_slice_test.cc inlined_vector_test.cc soa_test.cc status_test.cpp)

add_library(jpp_util ${jpp_util_sources} ${jpp_util_headers})
jpp_test_executable(jpp_util_test ${jpp_util_test_srcs} ${jpp_util_headers})
target_link_libraries(jpp_util_test jpp_util)
