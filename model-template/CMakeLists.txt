configure_file(jumandic.conf.in ${CMAKE_CURRENT_BINARY_DIR}/jumandic.config)

install(FILES jumandic.jppmdl
  DESTINATION libexec/jumanpp/
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/jumandic.config
  DESTINATION ${JPP_DEFAULT_CONFIG_DIR}
)