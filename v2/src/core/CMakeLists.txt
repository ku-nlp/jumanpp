set(jpp_core_cfg_dir ${CMAKE_CURRENT_BINARY_DIR}/cfg)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/core_config.h.in
        ${jpp_core_cfg_dir}/core_config.h
)

function(jpp_add_core_objlib NAME SRC)
    add_library(${NAME} OBJECT ${${SRC}})
    target_include_directories(${NAME} PRIVATE ${jpp_core_cfg_dir} ${JPP_SRC_DIR})
endfunction()

add_subdirectory(analysis)
add_subdirectory(api)
add_subdirectory(dic)
add_subdirectory(impl)
add_subdirectory(spec)
add_subdirectory(training)

set(core_srcs

        core.cc
        darts_trie.cc
        dic_builder.cc
        dictionary.cc
        env.cc
        features_api.cc

        )

set(core_hdrs
        ${CMAKE_CURRENT_BINARY_DIR}/core_config.h
        core.h
        core_types.h
        darts.h
        darts_trie.h
        dic_builder.h
        dic_entries.h
        dictionary.h
        env.h
        features_api.h
        runtime_info.h)

set(core_test_srcs
        darts_trie_test.cc
        dictionary_test.cc
        runtime_info_test.cc)

set(core_objs
        $<TARGET_OBJECTS:jpp_core_analysis>
        $<TARGET_OBJECTS:jpp_core_impl>
        $<TARGET_OBJECTS:jpp_core_spec>)

add_library(jpp_core ${core_srcs} ${core_hdrs} ${libs3p_pegtl_headers} ${core_objs})

target_include_directories(jpp_core PUBLIC ${jpp_core_cfg_dir})

jpp_test_executable(jpp_core_tests ${core_test_srcs} $<TARGET_OBJECTS:jpp_core_impl>)

target_link_libraries(jpp_core jpp_util jpp_rnn)
target_link_libraries(jpp_core_tests jpp_core jpp_core_train)

add_subdirectory(benchmarks)
if (${JPP_ENABLE_DEV_TOOLS})
    add_subdirectory(devtools)
endif()