add_executable(codegen_tests_main_1 cg_1_main.cc)
target_link_libraries(codegen_tests_main_1 jpp_core)

add_benchmark(codegen_bench_01 codegen_bench_01.cc jpp_core)

jpp_feature_codegen(
        BASE cgtest02
        CLASS Test02
        SPEC_INCLUDE cg_2_spec.h
        SPEC_NAME jumanpp::codegentest::CgTwoSpecFactory
        LINK jpp_core
        DEPEND cg_2_spec.h
)

jpp_test_executable(codegen_tests cg_simple_test.cc ${cgtest02_SRC})
target_include_directories(codegen_tests PRIVATE ${cgtest02_INCLUDE})
target_link_libraries(codegen_tests jpp_core)