set(jpp_rnn_sources mikolov_rnn.cc)
set(jpp_rnn_includes mikolov_rnn.h)
set(jpp_rnn_tests mikolov_rnn_test.cc)

add_library(jumanpp_rnn ${jpp_rnn_sources} ${jpp_rnn_includes} )

jpp_test_executable(jpp_rnn_tests ${jpp_rnn_tests})
target_link_libraries(jumanpp_rnn jpp_util)
target_link_libraries(jpp_rnn_tests jumanpp_rnn)