cmake_minimum_required(VERSION 3.1)
project(jumanpp)

include(version.cmake)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
    include(cmake/GitUpdate.cmake)
    include(cmake/GitVersion.cmake)
    set(JUMANPP_FULL_VERSION "${PROJECT_VERSION}${GIT_VERSION_STRING}")
else()
    set(JUMANPP_FULL_VERSION "${PROJECT_VERSION}")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dependencies.sh)
    execute_process(COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/dependencies.sh)
endif()

include_directories("${CMAKE_SOURCE_DIR}/libs")

option(JPP_TEST_REPORT_JUNIT OFF)
option(JPP_ENABLE_BENCHMARKS "Enable benchmarks" OFF)
option(JPP_ENABLE_DEV_TOOLS "Enable development-only binaries" OFF)
option(JPP_PREFETCH_FEATURE_WEIGHTS "Prefetch linear model weights when computing features" ON)
set(JPP_MAX_DIC_FIELDS 16 CACHE STRING "Maximum supported dictionary fields")
option(JPP_ENABLE_TESTS "Enable tests" ON)
set(JPP_DEFAULT_CONFIG_DIR "${CMAKE_INSTALL_PREFIX}/libexec/jumanpp/"
  CACHE FILEPATH "Default Config Location")
option(JPP_TRAIN_MID_NGRAMS OFF)
option(JPP_TRAIN_VIOLATION_INVALID ON)

if(${JPP_ENABLE_TESTS})
    enable_testing()
endif()

find_package(Threads)
set(Protobuf_USE_STATIC_LIBS ON)
find_package(Protobuf)

add_subdirectory(libs)
add_subdirectory(test) #need to set variable here
add_subdirectory(src)
if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/model)
    add_subdirectory(model)
endif()