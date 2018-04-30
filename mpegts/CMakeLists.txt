cmake_minimum_required (VERSION 2.6)
project (mpegts)

set(CMAKE_CXX_FLAGS "-g -std=c++11 -Wall")

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mpegts)

set (SRC_FILE_LISTS )
include_directories(${SRC_DIR})
aux_source_directory(${SRC_DIR} SRC_FILE_LISTS)

add_executable(mpegts.out ${SRC_FILE_LISTS} ${SRC_DIR}/main.cpp)

