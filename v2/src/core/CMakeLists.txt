set(core_srcs darts_trie.cc dictionary.cc dic_spec.cc impl/field_import.cc dic_builder.cc)
set(core_hdrs darts.h darts_trie.h dictionary.h dic_spec.h impl/field_import.h impl/field_reader.h dic_entries.h
        dic_builder.h spec/spec_grammar.h)
set(core_test_srcs darts_trie_test.cc dictionary_test.cc dic_spec_test.cc impl/field_import_test.cc
        impl/field_reader_test.cc spec/spec_grammar_test.cc)

add_library(jpp_core ${core_srcs} ${core_hdrs} ${libs3p_pegtl_headers})

jpp_test_executable(jpp_core_tests ${core_test_srcs} ${core_hdrs})

target_link_libraries(jpp_core jpp_util)
target_link_libraries(jpp_core_tests jpp_core jpp_util)