include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")


# registering tests as
# jpp_test(something_test.cc dependency1 dependency2)


set(JPP_TEST_RUNDIR ${CMAKE_CURRENT_SOURCE_DIR}/test)

function(jpp_test source)
    set(EXECUTABLE_NAME ${source}.bin)

    add_executable(${EXECUTABLE_NAME} ${source})

    target_link_libraries(${EXECUTABLE_NAME} ${ARGN})

    if (JPP_TEST_REPORT_JUNIT)
        set(CATCH_FLAGS -r junit -o "${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE_NAME}.report.xml")
    else(JPP_TEST_REPORT_JUNIT)
        set(CATCH_FLAGS "")
    endif(JPP_TEST_REPORT_JUNIT)

    add_test(
            NAME ${EXECUTABLE_NAME}
            COMMAND ${EXECUTABLE_NAME} ${CATCH_FLAGS}
            WORKING_DIRECTORY ${JPP_TEST_RUNDIR})
endfunction()


add_subdirectory(src)
add_subdirectory(test)