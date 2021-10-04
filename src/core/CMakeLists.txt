set(jpp_core_cfg_dir ${CMAKE_CURRENT_BINARY_DIR}/cfg)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/core_config.h.in
  ${jpp_core_cfg_dir}/core_config.h
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/core_version.h.in
  ${jpp_core_cfg_dir}/core_version.h
)

macro(jpp_core_files NAME)
  foreach (file ${ARGN})
    list(APPEND ${NAME} ${CMAKE_CURRENT_SOURCE_DIR}/${file})
  endforeach ()
  set(${NAME} ${${NAME}} PARENT_SCOPE)
endmacro()


set(core_srcs)
set(core_tsrcs)
set(core_hdrs)


if (${JPP_USE_PROTOBUF})
  protobuf_generate_cpp(core_srcs core_hdrs proto/lattice_dump.proto)
  add_subdirectory(proto)
endif()

add_subdirectory(analysis)
add_subdirectory(api)
add_subdirectory(codegen)
add_subdirectory(dic)
add_subdirectory(impl)
add_subdirectory(input)
add_subdirectory(spec)
add_subdirectory(tool)
add_subdirectory(training)

set(core_srcs
  core.cc
  env.cc
  features_api.cc
  ${core_srcs}
  )

set(core_hdrs
  ${jpp_core_cfg_dir}/core_config.h
  ${jpp_core_cfg_dir}/core_version.h
  core.h
  core_types.h
  env.h
  features_api.h
  ${core_hdrs}
        impl/feature_impl_hasher.h)

set(core_test_srcs
  ${core_test_srcs}
  ${core_tsrcs}
  test/test_analyzer_env.h
  ../testing/test_analyzer.h
  )

add_library(jpp_core ${core_srcs} ${core_hdrs} ${libs3p_pegtl_headers})
jpp_test_executable(jpp_core_tests ${core_test_srcs})
if(JPP_ENABLE_TESTS)
  target_link_libraries(jpp_core_tests jpp_core jpp_core_train)
endif()


target_include_directories(jpp_core PUBLIC ${jpp_core_cfg_dir})

target_link_libraries(jpp_core PUBLIC jpp_util jpp_rnn PRIVATE pathie)

if (${JPP_USE_PROTOBUF})
  target_include_directories(jpp_core PUBLIC ${Protobuf_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(jpp_core PRIVATE ${Protobuf_LIBRARY})
endif()

add_subdirectory(benchmarks)
if (${JPP_ENABLE_DEV_TOOLS})
  add_subdirectory(devtools)
endif ()