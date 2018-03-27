set(jumandic_headers shared/juman_format.h main/jumanpp.h shared/jumanpp_args.h
  shared/jumandic_env.h shared/morph_format.h shared/jumandic_ids.h shared/jumandic_id_resolver.h shared/jpp_rnn_args.h
  shared/mdic_format.h shared/subset_format.h shared/lattice_format.h)

set(jumandic_sources shared/juman_format.cc
  shared/jumandic_env.cc shared/jumandic_test_env.h shared/morph_format.cc shared/jumandic_ids.cc
  shared/jumandic_id_resolver.cc shared/mdic_format.cc shared/subset_format.cc
  shared/lattice_format.cc shared/jumanpp_args.cc)

set(jumandic_tests shared/jumandic_spec_test.cc shared/mini_dic_test.cc shared/training_test.cc
  shared/mdic_format_test.cc tests/partial_data_train.cc shared/jumandic_codegen_test.cc
  tests/unk_node_match_test.cc)

set(bug_test_sources tests/bug_950111-003_test.cc tests/bug_28_lattice.cc)

add_library(jpp_jumandic_spec shared/jumandic_spec.cc shared/jumandic_spec_lexdata.cc shared/jumandic_spec.h)
target_link_libraries(jpp_jumandic_spec jpp_core)

jpp_feature_codegen(
  BASE jpp_jumandic_cg
  CLASS JumandicStatic
  SPEC_INCLUDE shared/jumandic_spec.h
  SPEC_NAME jumanpp::jumandic::SpecFactory
  LINK jpp_jumandic_spec
  DEPEND shared/jumandic_spec.cc
)

if (${JPP_USE_PROTOBUF})
  list(APPEND jumandic_tests shared/jumandic_debug_dump_test.cc)
  list(APPEND jumandic_headers ${jumandic_pb_hdrs})
  list(APPEND jumandic_sources ${jumandic_pb_srcs})
endif ()

message(${jpp_jumandic_cg_INCLUDE})

if (${JPP_USE_PROTOBUF})
  JPP_PROTOBUF_COMPILE_CPP(jpp_proto_srcs jpp_proto_hdrs
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen
    proto/juman.proto proto/jumanpp.proto)
  list(APPEND jumandic_sources ${jpp_proto_srcs} shared/jumanpp_pb_format.cc shared/juman_pb_format.cc)
  list(APPEND jumandic_headers ${jpp_proto_hdrs} shared/jumanpp_pb_format.h shared/juman_pb_format.h)
endif()

add_library(jpp_jumandic ${jumandic_sources} ${jumandic_headers} ${jpp_jumandic_cg_SRC})
target_include_directories(jpp_jumandic PRIVATE ${jpp_jumandic_cg_INCLUDE})
if (${JPP_USE_PROTOBUF})
  target_include_directories(jpp_jumandic PUBLIC ${Protobuf_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/gen)
  target_link_libraries(jpp_jumandic ${Protobuf_LIBRARIES})
  set_target_properties(jpp_jumandic
    PROPERTIES PROTOBUF_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/proto;${JPP_SRC_DIR}/core/proto"
  )
endif ()


jpp_test_executable(jpp_jumandic_tests ${jumandic_tests})
jpp_test_executable(jpp_bug_tests ${bug_test_sources})
target_include_directories(jpp_jumandic_tests PRIVATE ${jpp_jumandic_cg_INCLUDE})

add_executable(jpp_jumandic_bootstrap main/bootstrap.cc)
add_executable(jumanpp_v2 main/jumanpp.cc)
add_executable(jumanpp_v2_train main/jumanpp_train.cc main/jumanpp_train.h)
add_executable(jpp_jumandic_pathdiff main/path_diff.cc)
target_include_directories(jpp_jumandic_pathdiff PRIVATE ${jpp_jumandic_cg_INCLUDE})

target_link_libraries(jpp_jumandic jpp_jumandic_spec)
target_link_libraries(jpp_jumandic_tests jpp_jumandic jpp_core_train)
target_link_libraries(jpp_bug_tests jpp_jumandic jpp_core_train)
target_link_libraries(jpp_jumandic_bootstrap jpp_jumandic)
target_link_libraries(jumanpp_v2 jpp_jumandic)
target_link_libraries(jumanpp_v2_train jpp_jumandic jpp_core_train)
target_link_libraries(jpp_jumandic_pathdiff jpp_jumandic)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/jumanpp_v2 RENAME jumanpp DESTINATION bin)