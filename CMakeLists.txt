cmake_minimum_required(VERSION 3.10)
project(jumanpp)

include(version.cmake)

if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/GitUpdate.cmake)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/GitVersion.cmake)
    set(JUMANPP_FULL_VERSION "${PROJECT_VERSION}${GIT_VERSION_STRING}")
    message(${JUMANPP_FULL_VERSION})
else()
    set(JUMANPP_FULL_VERSION "${PROJECT_VERSION}")
endif()
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/version ${JUMANPP_FULL_VERSION})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


if (MSVC)
	add_compile_options(
    "/utf-8" #Ensures MSVC treats strings as UTF-8 at both compile/runtime time
    "/wd4141" # double inline
    "/wd4018" # comparing signed-unsigned types
    "/wd4200" # nonstandard extension used: zero-sized array in struct/union
  )
endif()

if (WIN32)
    add_definitions("-D_WIN32_WINNT=0x0600") # VISTA
endif()

include_directories("${CMAKE_SOURCE_DIR}/libs")

if (NOT JPP_MAX_DIC_FIELDS)
set(JPP_MAX_DIC_FIELDS 16)
endif()

option(JPP_TEST_REPORT_JUNIT OFF)
option(JPP_ENABLE_BENCHMARKS "Enable benchmarks" OFF)
option(JPP_ENABLE_DEV_TOOLS "Enable development-only binaries" OFF)
option(JPP_PREFETCH_FEATURE_WEIGHTS "Prefetch linear model weights when computing features" ON)
set(JPP_MAX_DIC_FIELDS ${JPP_MAX_DIC_FIELDS} CACHE STRING "Maximum supported dictionary fields")
option(JPP_ENABLE_TESTS "Enable tests" ON)
set(JPP_DEFAULT_CONFIG_DIR "${CMAKE_INSTALL_PREFIX}/libexec/jumanpp/"
  CACHE FILEPATH "Default Config Location")
option(JPP_TRAIN_MID_NGRAMS "Train mid ngrams" OFF)
option(JPP_TRAIN_VIOLATION_INVALID "Train invalid violation" ON)
option(JPP_USE_PROTOBUF "Enable Protobuf-based components" ON)

if(${JPP_ENABLE_TESTS})
    enable_testing()
endif()

find_package(Threads)
set(Protobuf_USE_STATIC_LIBS ON)
find_package(Protobuf)
message("Detecting protobuf... ${Protobuf_FOUND}")
message("protobuf library: ${Protobuf_VERSION}")
message("protoc binary: ${Protobuf_PROTOC_EXECUTABLE}")
if ((NOT EXISTS ${Protobuf_PROTOC_EXECUTABLE})
    OR (NOT ${Protobuf_FOUND})
    OR (${Protobuf_VERSION} VERSION_LESS "3.1.0")
    OR (NOT ${JPP_USE_PROTOBUF}))
    set(JPP_USE_PROTOBUF OFF)
    message("Not using Protobuf-based components")
else()
    message("Using Protobuf-based components. Pass -DJPP_USE_PROTOBUF=OFF to disable.")
    include(cmake/ProtobufCompile.cmake)
endif()

add_subdirectory(libs)
add_subdirectory(test) #need to set variable here
add_subdirectory(src)
if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/model)
    add_subdirectory(model)
endif()

if (JPP_USE_CPACK)
    include(cmake/CPackJpp.cmake)
endif()