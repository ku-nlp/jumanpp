jpp_core_files(core_srcs

  partial_example.cc
  partial_example_io.cc
  pex_stream_reader.cc
  stream_reader.cc
  training_io.cc
  )

set(core_input_hdrs

  partial_example.h
  partial_example_io.h
  pex_stream_reader.h
  stream_reader.h
  training_io.h
  )
install(FILES ${core_input_hdrs}
  DESTINATION "${JPP_INSTALL_INCLUDE_DIR}/core/input")
jpp_core_files(core_hdrs ${core_input_hdrs})

jpp_core_files(core_tsrcs
  partial_example_io_test.cc
  )