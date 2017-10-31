set(core_spec_src

        spec_dsl.cc
        spec_serialization.cc
        spec_types.cc

        )

set(core_spec_tsrc

        spec_dsl_feature_test.cc
        spec_dsl_kvlist_test.cc
        spec_dsl_test.cc
        spec_grammar_test.cc

        )

set(core_spec_hdrs

        spec_dsl.h
        spec_grammar.h
        spec_ser.h
        spec_serialization.h
        spec_types.h

        )


jpp_add_core_objlib(jpp_core_spec core_spec_src)
jpp_test_executable(jpp_core_spec_tests ${core_spec_tsrc})
target_link_libraries(jpp_core_spec_tests jpp_core)