set(jpp_util_sources mmap.cc memory.cpp logging.cpp string_piece.cc status.cpp
  csv_reader.cc coded_io.cc characters.cc printer.cc codegen.cc assert.cc format.cc)

if(MSVC)
    set(jpp_util_sources ${jpp_util_sources} win32_utils.cc)
endif()

set(jpp_util_headers mmap.h status.hpp memory.hpp characters.h types.hpp logging.hpp common.hpp
  string_piece.h csv_reader.h coded_io.h flatrep.h flatset.h flatmap.h murmur_hash.h array_slice_internal.h
  array_slice.h inlined_vector.h stl_util.h hashing.h char_buffer.h serialization.h
  sliceable_array.h printer.h codegen.h array_slice_util.h lazy.h debug_output.h
  seahash.h serialization_flatmap.h lru_cache.h bounded_queue.h fast_hash.h assert.h
  quantized_weights.h format.h fast_printer.h cfg.h)
set(jpp_util_test_srcs memory_test.cpp mmap_test.cc string_piece_test.cc
  csv_reader_test.cc coded_io_test.cc characters_test.cpp hashing_test.cc
  array_slice_test.cc inlined_vector_test.cc status_test.cpp
  serialization_test.cc printer_test.cc array_slice_util_test.cc lazy_test.cc
  seahash_test.cc fast_hash_test.cc stl_util_test.cc
  )

add_library(jpp_util ${jpp_util_sources} ${jpp_util_headers} ${BACKWARD_headers})
jpp_test_executable(jpp_util_test ${jpp_util_test_srcs} ${jpp_util_headers})
target_link_libraries(jpp_util_test jpp_util)
