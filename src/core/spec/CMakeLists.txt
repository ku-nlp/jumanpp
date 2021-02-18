jpp_core_files(core_srcs

  spec_compiler.cc
  spec_dsl.cc
  spec_hashing.cc
  spec_parser.cc
  spec_parser_impl.cc
  spec_serialization.cc
  spec_types.cc

  )

set(core_spec_tsrc

  spec_dsl_feature_test.cc
  spec_dsl_test.cc
  spec_grammar_test.cc
  spec_parser_impl_test.cc

  )

jpp_core_files(core_hdrs

  spec_compiler.h
  spec_dsl.h
  spec_grammar.h
  spec_hashing.h
  spec_parser.h
  spec_parser_impl.h
  spec_ser.h
  spec_serialization.h
  spec_types.h

  )

jpp_test_executable(jpp_core_spec_tests ${core_spec_tsrc} ${libs3p_pegtl_headers})
if(JPP_ENABLE_TESTS)
  target_link_libraries(jpp_core_spec_tests jpp_core)
endif()