set(jumandic_headers shared/jumandic_spec.h)
set(jumandic_sources shared/jumandic_spec.cc shared/jumandic_spec_lexdata.cc )
set(jumandic_tests shared/jumandic_spec_test.cc shared/mini_dic_test.cc)

add_library(jpp_jumandic ${jumandic_sources} ${jumandic_headers})
jpp_test_executable(jpp_jumandic_tests ${jumandic_tests})

add_executable(jpp_jumandic_bootstrap main/bootstrap.cc)

target_link_libraries(jpp_jumandic jpp_core)
target_link_libraries(jpp_jumandic_tests jpp_jumandic)
target_link_libraries(jpp_jumandic_bootstrap jpp_jumandic)

