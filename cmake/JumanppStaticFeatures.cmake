function(jumanpp_gen_static spec class dir_var sources_var)
  set(CODEGEN_OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/codegen/")
  set(${dir_var} ${CODEGEN_OUTDIR} PARENT_SCOPE)
  get_filename_component(CG_SPEC_NAME ${spec} NAME)
  set(CG_BASE_NAME "${CODEGEN_OUTDIR}/${CG_SPEC_NAME}")
  set(CG_OUT_SRC "${CG_BASE_NAME}.cc")
  set(CG_OUT_HDR "${CG_BASE_NAME}.h")
  set(${sources_var} ${CG_OUT_SRC} ${CG_OUT_HDR} PARENT_SCOPE)
  add_custom_command(
    COMMAND jumanpp_tool
    ARGS static-features "--spec=${spec}" "--class-name=${class}" "--output=${CG_BASE_NAME}"
    DEPENDS ${spec} ${jumanpp_tool}
    OUTPUT ${CG_OUT_SRC} ${CG_OUT_HDR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endfunction()