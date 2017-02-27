set(core_srcs darts_trie.cc dictionary.cc impl/field_import.cc dic_builder.cc
        analysis/dictionary_node_creator.cc analysis/lattice_types.cc
        analysis/lattice_builder.cc analysis/analysis_input.cc spec/spec_dsl.cc)
set(core_hdrs darts.h darts_trie.h dictionary.h impl/field_import.h impl/field_reader.h dic_entries.h
        dic_builder.h spec/spec_grammar.h spec/spec_dsl.h analysis/analysis_input.h analysis/lattice_builder.h
        analysis/dictionary_node_creator.h spec/spec_types.h analysis/lattice_types.h)
set(core_test_srcs darts_trie_test.cc dictionary_test.cc impl/field_import_test.cc
        impl/field_reader_test.cc )
set(spec_test_srcs spec/spec_grammar_test.cc spec/spec_dsl_test.cc)
set(analysis_test_srcs analysis/dictionary_node_creator_test.cc analysis/lattice_types_test.cc)

add_library(jpp_core ${core_srcs} ${core_hdrs} ${libs3p_pegtl_headers})

jpp_test_executable(jpp_core_tests ${core_test_srcs} ${core_hdrs})
jpp_test_executable(jpp_core_analysis_tests ${analysis_test_srcs})
jpp_test_executable(jpp_core_spec_tests ${spec_test_srcs})

target_link_libraries(jpp_core jpp_util)
target_link_libraries(jpp_core_tests jpp_core jpp_util)
target_link_libraries(jpp_core_analysis_tests jpp_core jpp_util)
target_link_libraries(jpp_core_spec_tests jpp_core jpp_util)