set(libs3p_catch_header ${CMAKE_CURRENT_SOURCE_DIR}/catch.hpp PARENT_SCOPE)
file(GLOB pegtl_headers_raw ${CMAKE_CURRENT_SOURCE_DIR}/pegtl/*.hh ${CMAKE_CURRENT_SOURCE_DIR}/pegtl/*/*.hh)
set(libs3p_pegtl_headers ${pegtl_headers_raw} PARENT_SCOPE)
set(BACKWARD_headers ${CMAKE_CURRENT_SOURCE_DIR}/backward.hpp PARENT_SCOPE)

set(EIGEN_VERSION "3.3.4")
set(EIGEN_HASH "dd254beb0bafc695d0f62ae1a222ff85b52dbaa3a16f76e781dce22d0d20a4a6")

if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/eigen3)
  message("-- Eigen3... OK")
else()
  file(DOWNLOAD
    "https://bitbucket.org/eigen/eigen/get/${EIGEN_VERSION}.tar.bz2"
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
  file(RENAME ${EIGEN_DIRNAMES} ${CMAKE_CURRENT_SOURCE_DIR}/eigen3)
endif()