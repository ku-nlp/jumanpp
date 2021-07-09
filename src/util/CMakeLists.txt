set(jpp_util_sources mmap.cc memory.cpp logging.cpp string_piece.cc status.cpp
  csv_reader.cc coded_io.cc characters.cc printer.cc codegen.cc assert.cc format.cc
  parse_utils.cc
  )

set(jpp_util_headers mmap.h status.hpp memory.hpp characters.h types.hpp logging.hpp common.hpp
  string_piece.h csv_reader.h coded_io.h flatrep.h flatset.h flatmap.h murmur_hash.h array_slice_internal.h
  array_slice.h inlined_vector.h stl_util.h hashing.h char_buffer.h serialization.h
  sliceable_array.h printer.h codegen.h array_slice_util.h lazy.h debug_output.h
  seahash.h serialization_flatmap.h lru_cache.h bounded_queue.h fast_hash.h assert.h
  quantized_weights.h format.h fast_printer.h cfg.h mmap_impl_unix.h  mmap_impl_win32.h
  parse_utils.h fast_hash_rot.h)

set(jpp_util_test_srcs memory_test.cpp mmap_test.cc string_piece_test.cc
  csv_reader_test.cc coded_io_test.cc characters_test.cpp hashing_test.cc
  array_slice_test.cc inlined_vector_test.cc status_test.cpp
  serialization_test.cc printer_test.cc array_slice_util_test.cc lazy_test.cc
  seahash_test.cc fast_hash_test.cc stl_util_test.cc parse_utils_test.cc
  )

if(WIN32)
    list(APPEND jpp_util_sources win32_utils.cc)
    list(APPEND jpp_util_headers win32_utils.h)
endif()


add_library(jpp_util ${jpp_util_sources} ${jpp_util_headers} ${BACKWARD_headers})
jpp_test_executable(jpp_util_test ${jpp_util_test_srcs} ${jpp_util_headers})
target_link_libraries(jpp_util_test jpp_util)
target_link_libraries(jpp_util ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(jpp_util PUBLIC ${JPP_LIBS_DIR} ${JPP_SRC_DIR})
target_compile_features(jpp_util PUBLIC
  cxx_constexpr
  cxx_variadic_templates
  cxx_alias_templates
  cxx_alignas
  cxx_alignof
  cxx_auto_type
  cxx_decltype
  cxx_defaulted_functions
  cxx_final
  cxx_lambdas
  cxx_noexcept
  cxx_nullptr
  cxx_range_for
  cxx_strong_enums
  cxx_unrestricted_unions
  cxx_uniform_initialization

  )

if (NOT MSVC)
    target_compile_features(jpp_util PUBLIC
      cxx_relaxed_constexpr # only C++14 feature we use now
  )
endif()
