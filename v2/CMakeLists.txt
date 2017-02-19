include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")


# registering tests as
# jpp_test(something_test.cc dependency1 dependency2)


set(JPP_TEST_RUNDIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
set(JPP_TEST_STANDALONE_HDRS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/testing/standalone_test.h)

add_library(jpp_test_catch_main_lib ${CMAKE_CURRENT_SOURCE_DIR}/src/testing/catch_main.cc)

function(jpp_register_test executable)
    if (JPP_TEST_REPORT_JUNIT)
        set(CATCH_FLAGS -r junit -o "${CMAKE_CURRENT_BINARY_DIR}/${executable}.report.xml")
    else(JPP_TEST_REPORT_JUNIT)
        set(CATCH_FLAGS "")
    endif(JPP_TEST_REPORT_JUNIT)

    add_test(
            NAME ${executable}
            COMMAND ${executable} ${CATCH_FLAGS}
            WORKING_DIRECTORY ${JPP_TEST_RUNDIR})
endfunction()

function(jpp_test_executable executable_name)
    set(TEST_SOURCES ${ARGN})
    add_executable(${executable_name} ${TEST_SOURCES} ${JPP_TEST_STANDALONE_HDRS} ${libs3p_catch_header})
    target_link_libraries(${executable_name} jpp_test_catch_main_lib)
    jpp_register_test(${executable_name})
endfunction()




add_subdirectory(src)
add_subdirectory(test)