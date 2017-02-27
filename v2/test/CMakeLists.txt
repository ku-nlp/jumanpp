set(JPP_TEST_SOURCES
        fixed_min_queue_test.cpp)


jpp_test_executable(misc_tests ${JPP_TEST_SOURCES})
target_link_libraries(misc_tests jpp_util)