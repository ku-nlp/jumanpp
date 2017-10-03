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
option(ENABLE_BENCHMARKS OFF)
set(JPP_MAX_DIC_FIELDS 64 CACHE STRING "Maximum supported dictionary fields")

enable_testing()

find_package(Threads)

add_subdirectory(libs)
add_subdirectory(src)
add_subdirectory(v2)