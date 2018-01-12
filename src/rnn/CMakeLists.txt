set(jpp_rnn_sources mikolov_rnn.cc)
set(jpp_rnn_includes mikolov_rnn.h simple_rnn_impl.h mikolov_rnn_impl.h)
set(jpp_rnn_tests mikolov_rnn_test.cc)

add_library(jpp_rnn ${jpp_rnn_sources} ${jpp_rnn_includes})
target_link_libraries(jpp_rnn Eigen3)
add_library(jumanpp_rnn_legacy legacy/rnnlmlib.h legacy/rnnlmlib_static.h legacy/rnnlmlib_static.cpp)

jpp_test_executable(jpp_rnn_tests ${jpp_rnn_tests})
target_link_libraries(jpp_rnn jpp_util)
target_link_libraries(jpp_rnn_tests jpp_rnn jumanpp_rnn_legacy)
