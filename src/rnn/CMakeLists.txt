set(jpp_rnn_sources mikolov_rnn.cc)
set(jpp_rnn_includes mikolov_rnn.h simple_rnn_impl.h mikolov_rnn_impl.h)
install(FILES ${jpp_rnn_inclues}
  DESTINATION "${JPP_INSTALL_INCLUDE_DIR}/rnn")
set(jpp_rnn_tests mikolov_rnn_test.cc)

add_library(jpp_rnn ${jpp_rnn_sources} ${jpp_rnn_includes} )
install(TARGETS jpp_rnn
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
add_library(jumanpp_rnn_legacy legacy/rnnlmlib.h legacy/rnnlmlib_static.h legacy/rnnlmlib_static.cpp)

jpp_test_executable(jpp_rnn_tests ${jpp_rnn_tests})
target_link_libraries(jpp_rnn jpp_util)
target_link_libraries(jpp_rnn_tests jpp_rnn jumanpp_rnn_legacy)

target_link_libraries(jumanpp_rnn_legacy jpp_util)