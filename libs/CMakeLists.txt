set(libs3p_catch_header ${CMAKE_CURRENT_SOURCE_DIR}/catch.hpp PARENT_SCOPE)
file(GLOB pegtl_headers_raw ${CMAKE_CURRENT_SOURCE_DIR}/pegtl/*.hh ${CMAKE_CURRENT_SOURCE_DIR}/pegtl/*/*.hh)
set(libs3p_pegtl_headers ${pegtl_headers_raw} PARENT_SCOPE)