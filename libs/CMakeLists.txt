set(libs3p_catch_header ${CMAKE_CURRENT_SOURCE_DIR}/catch.hpp PARENT_SCOPE)
file(GLOB_RECURSE pegtl_headers_raw ${CMAKE_CURRENT_SOURCE_DIR}/pegtl/ *.hpp)
set(libs3p_pegtl_headers ${pegtl_headers_raw} PARENT_SCOPE)
set(BACKWARD_headers ${CMAKE_CURRENT_SOURCE_DIR}/backward.hpp PARENT_SCOPE)
set(JPP_LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

set(EIGEN_VERSION "3.3.7")
set(EIGEN_HASH "685adf14bd8e9c015b78097c1dc22f2f01343756f196acdc76a678e1ae352e11")

if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/eigen3)
  message("-- Eigen3... OK")
else()
  file(DOWNLOAD
      "https://gitlab.com/libeigen/eigen/-/archive/${EIGEN_VERSION}/eigen-${EIGEN_VERSION}.tar.bz2"
    "${CMAKE_CURRENT_BINARY_DIR}/eigen.tar.bz2"
    EXPECTED_HASH SHA256=${EIGEN_HASH}
    )

  set(EIGEN_TMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/eigen_tmp")

  file(REMOVE_RECURSE ${EIGEN_TMP_DIR})
  file(MAKE_DIRECTORY ${EIGEN_TMP_DIR})

  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xf "${CMAKE_CURRENT_BINARY_DIR}/eigen.tar.bz2"
    WORKING_DIRECTORY ${EIGEN_TMP_DIR}
    RESULT_VARIABLE EIGEN_EXTRACT_OK
  )

  if (NOT EIGEN_EXTRACT_OK EQUAL 0)
    message("Failed to extract Eigen.")
  endif()

  file(GLOB EIGEN_DIRNAMES "${EIGEN_TMP_DIR}/*")
  file(COPY ${EIGEN_DIRNAMES} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/)
  file(RENAME ${CMAKE_CURRENT_SOURCE_DIR}/eigen-${EIGEN_VERSION} ${CMAKE_CURRENT_SOURCE_DIR}/eigen3)
  file(REMOVE_RECURSE ${EIGEN_DIRNAMES})
endif()

add_subdirectory(pathie-cpp EXCLUDE_FROM_ALL)

# Try to compile backward example

try_compile(RESULT_VAR ${CMAKE_CURRENT_BINARY_DIR}
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/backward-cfg/backward-test.cpp
        CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${CMAKE_CURRENT_SOURCE_DIR}
        CXX_STANDARD 14
        OUTPUT_VARIABLE BWD_OUT)

if (${RESULT_VAR})
  message("Backward... OK, no additional libraries needed")
  set(BACKWARD_libs "" PARENT_SCOPE)
  set(JPP_HAS_BACKWARD TRUE CACHE BOOL PARENT_SCOPE)
endif ()

# If failed, try to link with execinfo (for Alpine)

if (NOT ${RESULT_VAR})
  try_compile(RESULT_VAR ${CMAKE_CURRENT_BINARY_DIR}
          SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/backward-cfg/backward-test.cpp
          CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${CMAKE_CURRENT_SOURCE_DIR}
          LINK_LIBRARIES execinfo
          CXX_STANDARD 14
          OUTPUT_VARIABLE BWD_OUT)

  if (${RESULT_VAR})
    message("Backward... OK, linking execinfo additionally")
    set(BACKWARD_libs "execinfo" PARENT_SCOPE)
    set(JPP_HAS_BACKWARD TRUE CACHE BOOL PARENT_SCOPE)
  else()
    message("Backward... disabled")
    set(JPP_HAS_BACKWARD FALSE CACHE BOOL PARENT_SCOPE)
  endif ()
endif()