jpp_feature_codegen(
  BASE cgtest02
  CLASS Test02
  SPEC_INCLUDE cg_2_spec.h
  SPEC_NAME jumanpp::codegentest::CgTwoSpecFactory
  LINK jpp_core
  DEPEND cg_2_spec.h
)

set(jpp_codegen_srcs
  feature_codegen.cc
  ngram_feature_codegen.cc
  partial_ngram_feature_codegen.cc
  pattern_feature_codegen.cc
  )

set(jpp_codegen_hdrs
  feature_codegen.h
  ngram_feature_codegen.h
  partial_ngram_feature_codegen.h
  pattern_feature_codegen.h
  )

set(jpp_codegen_tsrcs
  cg_2_spec.h
  feature_codegen_test.cc
  )

add_library(jpp_core_codegen ${jpp_codegen_srcs} ${jpp_codegen_hdrs})

jpp_test_executable(jpp_codegen_tests ${jpp_codegen_tsrcs} ${cgtest02_SRC})
target_include_directories(jpp_codegen_tests PRIVATE ${cgtest02_INCLUDE})
target_link_libraries(jpp_core_codegen jpp_core)
target_link_libraries(jpp_codegen_tests jpp_core_codegen)