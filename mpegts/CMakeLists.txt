cmake_minimum_required (VERSION 2.6)
project (mpegts)

set(CMAKE_CXX_FLAGS "-g -std=c++11 -Wall")

set(AMF0_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../amf0/amf0)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mpegts)

set(SRC_FILE_LISTS mpegts/common.cpp mpegts/common.h mpegts/flv_muxer.cpp mpegts/flv_muxer.h ${AMF0_DIR}/amf_core.h ${AMF0_DIR}/amf0.h ${AMF0_DIR}/amf0.cpp ${AMF0_DIR}/amf_errno.h ${AMF0_DIR}/simple_buffer.h ${AMF0_DIR}/simple_buffer.cpp)
include_directories(${SRC_DIR} ${AMF0_DIR})
aux_source_directory(${SRC_DIR} SRC_FILE_LISTS)

add_executable(mpegts.out ${SRC_FILE_LISTS} ${SRC_DIR}/main.cpp)

