cmake_minimum_required(VERSION 3.1)
project(jumanpp VERSION 2.0.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

include(cmake/GitUpdate.cmake)
include(cmake/GitVersion.cmake)

set(JUMANPP_FULL_VERSION "${PROJECT_VERSION}${GIT_VERSION_STRING}")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

execute_process(COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/dependencies.sh)

include_directories("${CMAKE_SOURCE_DIR}/libs")

option(JPP_TEST_REPORT_JUNIT OFF)
option(JPP_ENABLE_BENCHMARKS "Enable benchmarks" OFF)
option(JPP_ENABLE_DEV_TOOLS "Enable development-only binaries" OFF)
option(JPP_PREFETCH_FEATURE_WEIGHTS "Prefetch linear model weights when computing features" ON)
set(JPP_MAX_DIC_FIELDS 16 CACHE STRING "Maximum supported dictionary fields")
option(JPP_ENABLE_TESTS "Enable tests" ON)
set(JPP_DEFAULT_CONFIG_DIR "${CMAKE_INSTALL_PREFIX}/libexec/jumanpp/"
  CACHE FILEPATH "Default Configu Location")

if(${JPP_ENABLE_TESTS})
    enable_testing()
endif()

find_package(Threads)

add_subdirectory(libs)
add_subdirectory(test) #need to set variable here
add_subdirectory(src)