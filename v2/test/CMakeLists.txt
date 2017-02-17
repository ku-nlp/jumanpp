set(JPP_TEST_SOURCES
        tests_main.cpp
        fixed_min_queue_test.cpp
        status_test.cpp
        memory_test.cpp
        test_characters.cpp)

add_executable(jpp_tests ${JPP_TEST_SOURCES} "../../libs/catch.hpp")

target_link_libraries(jpp_tests jpp_util)