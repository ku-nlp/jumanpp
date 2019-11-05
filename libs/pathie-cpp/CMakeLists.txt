# -*- mode: cmake; coding: utf-8 -*-
# This file is part of Pathie.
#
# Copyright © 2015, 2017 Marvin Gülker
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required(VERSION 3.1)
enable_language(CXX)

########################################
# Version number

set(PATHIE_VERSION_MAJOR 0)
set(PATHIE_VERSION_MINOR 1)
set(PATHIE_VERSION_PATCH 0)

#execute_process(COMMAND git rev-parse --short HEAD
#  WORKING_DIRECTORY ${Pathie_SOURCE_DIR}
#  OUTPUT_VARIABLE PATHIE_VERSION_GIT
#  OUTPUT_STRIP_TRAILING_WHITESPACE)

# If git was not available, unset empty variable
#if (NOT(PATHIE_VERSION_GIT))
#  unset(PATHIE_VERSION_GIT)
#endif()
set(PATHIE_VERSION_GIT "jumanpp")

########################################
# Flags & Options

option(PATHIE_BUILD_STREAM_REPLACEMENTS "Build the std::ifstream and std::ofstream replacements (experimental)." OFF)
option(PATHIE_ASSUME_UTF8_ON_UNIX "Assume file paths under UNIX to be always encoded in UTF-8." OFF)

########################################
# Extra flags

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra ")
endif()

# We only support Vista upwards. See http://msdn.microsoft.com/en-us/library/windows/desktop/aa383745(v=vs.85).aspx
# for the macro values.
if (WIN32)
  add_definitions("-D_WIN32_WINNT=0x0600") # Vista
  add_definitions("-D_WIN32_IE=0x0800") # IE 8.0+
  add_definitions("-DWINVER=0x0600") # Vista
elseif(UNIX)
  add_definitions("-D_PATHIE_UNIX=1")
  #add_definitions("-D_POSIX_C_SOURCE=200112L")
endif()

add_definitions("-DPATHIE_VERSION_MAJOR=${PATHIE_VERSION_MAJOR}")
add_definitions("-DPATHIE_VERSION_MINOR=${PATHIE_VERSION_MINOR}")
add_definitions("-DPATHIE_VERSION_PATCH=${PATHIE_VERSION_PATCH}")
if (PATHIE_VERSION_GIT)
  add_definitions("-DPATHIE_VERSION_GIT=\"${PATHIE_VERSION_GIT}\"")
endif()

if (PATHIE_ASSUME_UTF8_ON_UNIX)
  add_definitions("-DPATHIE_ASSUME_UTF8_ON_UNIX=1")
endif()

########################################
# Adjustments to cmake’s defaults

# set(CMAKE_BUILD_SHARED_LIBS On CACHE BOOL "Additionally to the static lib, build the shared lib")

########################################
# Source files

list(APPEND pathie_sources
  "src/errors.cpp"
  "src/entry_iterator.cpp"
  "src/path.cpp"
  "src/temp.cpp"
  "src/pathie.cpp")

if (PATHIE_BUILD_STREAM_REPLACEMENTS)
  list(APPEND pathie_sources
    "src/pathie_ifstream.cpp"
    "src/pathie_ofstream.cpp")
endif()

file(GLOB_RECURSE test_sources
  "test/*.cpp")

########################################
# Targets

# Libraries
add_library(pathie STATIC ${pathie_sources})
target_include_directories(pathie INTERFACE include)

#if (CMAKE_BUILD_SHARED_LIBS)
#  add_library(pathie-dynamic SHARED ${pathie_sources})
#  set_target_properties(pathie-dynamic PROPERTIES OUTPUT_NAME pathie)
#endif()

if(WIN32)
  target_link_libraries(pathie shlwapi)

#  if (CMAKE_BUILD_SHARED_LIBS)
#    target_link_libraries(pathie-dynamic shlwapi)
#  endif()
endif()

# Tests
# TODO: How to only do not test with crosscompilation?
#if (NOT(WIN32))
#  foreach(testfile ${test_sources})
#    get_filename_component(testtargetname "${testfile}" NAME_WE)
#    set(testtargetname "${testtargetname}.test")
#    message(STATUS "TESTFILE: ${testtargetname}")
#    #string(REPLACE ".cpp" ".test" testtargetname ${testfile})
#    add_executable(${testtargetname} ${testfile} ${Pathie_SOURCE_DIR}/test/testhelpers.hpp)
#  endforeach()
#endif()

########################################
# Installation information

#install(TARGETS pathie
#  DESTINATION lib)

#if (CMAKE_BUILD_SHARED_LIBS)
#  install(TARGETS pathie-dynamic
#    DESTINATION lib)
#endif()

#install(FILES
#  "include/errors.hpp"
#  "include/entry_iterator.hpp"
#  "include/path.hpp"
#  "include/temp.hpp"
#  "include/pathie.hpp"
#  DESTINATION include/pathie)
#
#if (PATHIE_BUILD_STREAM_REPLACEMENTS)
#  install(FILES
#    "include/pathie_ifstream.hpp"
#    "include/pathie_ofstream.hpp"
#    DESTINATION include/pathie)
#endif()

########################################
# Status message

#message(STATUS "--------------- Build info summary ----------------")
#message(STATUS "Pathie version:     ${PATHIE_VERSION_MAJOR}.${PATHIE_VERSION_MINOR}.${PATHIE_VERSION_PATCH}")
#message(STATUS "Build type:         ${CMAKE_BUILD_TYPE}")
#message(STATUS "Install prefix:     ${CMAKE_INSTALL_PREFIX}")
#if (CMAKE_CROSSCOMPILING)
#  message(STATUS "Crosscompiling:     Yes")
#else()
#  message(STATUS "Crosscompiling:     No")
#endif()
#if (UNIX)
#  message(STATUS "Assume UTF-8:       ${PATHIE_ASSUME_UTF8_ON_UNIX}")
#else()
#  message(STATUS "Assume UTF-8:       option unavailable on this target")
#endif()
#message(STATUS "Stream replacments: ${PATHIE_BUILD_STREAM_REPLACEMENTS}")
#message(STATUS "---------------------------------------------------")
