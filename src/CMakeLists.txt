function(add_benchmark name source)
    if (${JPP_ENABLE_BENCHMARKS})
        add_executable(${name} ${source})
        target_link_libraries(${name} ${ARGN})
    endif()
endfunction(add_benchmark)

set(JPP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(JPP_TEST_STANDALONE_HDRS
        ${JPP_SRC_DIR}/testing/standalone_test.h)

function(jpp_register_test executable)
    if (JPP_TEST_REPORT_JUNIT)
        set(CATCH_FLAGS -r junit "-#" --success "--out=${CMAKE_CURRENT_BINARY_DIR}/${executable}.report.xml")
    else(JPP_TEST_REPORT_JUNIT)
        set(CATCH_FLAGS "--use-colour" "yes")
    endif(JPP_TEST_REPORT_JUNIT)

    add_test(
            NAME ${executable}
            COMMAND ${executable} ${CATCH_FLAGS}
            WORKING_DIRECTORY ${JPP_TEST_RESOURCE_DIR}
            )
endfunction()

function(jpp_test_executable executable_name)
    if(JPP_ENABLE_TESTS)
      set(TEST_SOURCES ${ARGN})
      add_executable(${executable_name} ${TEST_SOURCES} ${JPP_TEST_STANDALONE_HDRS} ${libs3p_catch_header})
      target_link_libraries(${executable_name} jpp_test_catch_main_lib)
      jpp_register_test(${executable_name})
    endif()
endfunction()

function(jpp_feature_codegen)
    set(options )
    set(oneValueArgs BASE CLASS SPEC_INCLUDE SPEC_NAME)
    set(multiValueArgs DEPEND LINK)
    cmake_parse_arguments(CG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(CG_INPUT ${JPP_SRC_DIR}/core/codegen/codegen_main.cc.in)
    set(CG_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gen/${CG_BASE}_codegen_main.cc)
    configure_file(${CG_INPUT} ${CG_OUTPUT})
    set(CG_EXEC ${CG_BASE}_codegen_binary)
    add_executable(${CG_EXEC} ${CG_OUTPUT})
    target_link_libraries(${CG_EXEC} ${CG_LINK} jpp_core_codegen)
    target_include_directories(${CG_EXEC} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    set(CG_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)
    set(CG_OUT_SRC ${CG_OUT_DIR}/${CG_BASE}.cc)
    set(CG_OUT_HDR ${CG_OUT_DIR}/${CG_BASE}.h)
    file(MAKE_DIRECTORY ${CG_OUT_DIR})
    message("
    Static feature generation:
        COMMAND ${CG_EXEC} ${CG_BASE} ${CG_CLASS} ${CG_OUT_DIR}
        DEPENDS ${CG_EXEC} ${CG_DEPEND}
        OUTPUT ${CG_OUT_SRC} ${CG_OUT_HDR}
        LIBS ${CG_LINK}
    ")
    add_custom_command(
            COMMAND ${CG_EXEC}
            ARGS ${CG_BASE} ${CG_CLASS} ${CG_OUT_DIR}
            DEPENDS ${CG_DEPEND} jpp_core_codegen
            OUTPUT ${CG_OUT_SRC} ${CG_OUT_HDR}
    )
    set(${CG_BASE}_SRC ${CG_OUT_SRC} PARENT_SCOPE)
    set(${CG_BASE}_INCLUDE ${CG_OUT_DIR} PARENT_SCOPE)
endfunction(jpp_feature_codegen)

add_subdirectory(util)
add_subdirectory(testing)
add_subdirectory(core)
add_subdirectory(jumandic)
add_subdirectory(rnn)