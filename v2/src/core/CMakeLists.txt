set(core_srcs darts_trie.cc)
set(core_hdrs darts.h darts_trie.h)
set(core_test_srcs darts_trie_test.cc)

add_library(jpp_core ${core_srcs} ${core_hdrs})

jpp_test_executable(jpp_core_tests ${core_test_srcs} ${core_hdrs})

target_link_libraries(jpp_core jpp_util)
target_link_libraries(jpp_core_tests jpp_core jpp_util)