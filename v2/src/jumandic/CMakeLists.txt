set(jumandic_headers shared/jumandic_spec.h shared/juman_format.h main/jumanpp.h main/jumanpp_args.h
        shared/jumandic_env.h)
set(jumandic_sources shared/jumandic_spec.cc shared/jumandic_spec_lexdata.cc shared/juman_format.cc
        shared/jumandic_env.cc)
set(jumandic_tests shared/jumandic_spec_test.cc shared/mini_dic_test.cc main/jumanpp_test.cc shared/training_test.cc)

add_library(jpp_jumandic ${jumandic_sources} ${jumandic_headers})
jpp_test_executable(jpp_jumandic_tests ${jumandic_tests})

add_executable(jpp_jumandic_bootstrap main/bootstrap.cc)
add_executable(jumanpp_v2 main/jumanpp.cc main/jumanpp_args.cc)
add_executable(jumanpp_v2_train main/jumanpp_train.cc main/jumanpp_train.h)

target_link_libraries(jpp_jumandic jpp_core)
target_link_libraries(jpp_jumandic_tests jpp_jumandic jpp_core_train)
target_link_libraries(jpp_jumandic_bootstrap jpp_jumandic)
target_link_libraries(jumanpp_v2 jpp_jumandic)
target_link_libraries(jumanpp_v2_train jpp_jumandic jpp_core_train)

