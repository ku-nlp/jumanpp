set(testing_src read_file_test.cc)

jpp_test_executable(testing_infra_test ${testing_src})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/test_config.h ESCAPE_QUOTES)

add_library(jpp_test_catch_main_lib catch_main.cc)

target_include_directories(jpp_test_catch_main_lib PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
