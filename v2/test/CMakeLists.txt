set(JPP_TEST_SOURCES
        tests_main.cpp
        fixed_min_queue_test.cpp
        status_test.cpp
        test_characters.cpp)

add_executable(jpp_tests ${JPP_TEST_SOURCES} "../../libs/catch.hpp")

target_link_libraries(jpp_tests jpp_util)



function(jpp_register_test executable)
  if (JPP_TEST_REPORT_JUNIT)
    set(CATCH_FLAGS -r junit -o "${CMAKE_CURRENT_BINARY_DIR}/${executable}.report.xml")
  else(JPP_TEST_REPORT_JUNIT)
    set(CATCH_FLAGS "")
  endif(JPP_TEST_REPORT_JUNIT)
  
  add_test(
    NAME ${executable}
    COMMAND ${executable} ${CATCH_FLAGS} 
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()

#add_test(NAME common_tests COMMAND jpp_tests ${JPP_REPORT_FORMAT})

jpp_register_test(jpp_tests)