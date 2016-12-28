cmake_minimum_required(VERSION 3.0)
project(jumanpp)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g")

find_package(Boost "1.57" REQUIRED)

include_directories(${Boost_INCLUDE_DIRECTORIES})

add_subdirectory(src)