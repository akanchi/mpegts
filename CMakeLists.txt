cmake_minimum_required(VERSION 3.10)
project(mpegts)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

#Include amf0
include(ExternalProject)
ExternalProject_Add(project_amf0
        GIT_REPOSITORY https://github.com/akanchi/amf0.git
        GIT_SUBMODULES ""
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/amf0
        BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/amf0
        GIT_PROGRESS 1
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        EXCLUDE_FROM_ALL TRUE
        INSTALL_COMMAND ""
        )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mpegts/)

SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/amf0/amf0/amf0.cpp PROPERTIES GENERATED TRUE )
file(GLOB libfiles ${CMAKE_CURRENT_SOURCE_DIR}/mpegts/*.cpp)
add_library(mpegts STATIC ${libfiles} ${CMAKE_CURRENT_SOURCE_DIR}/amf0/amf0/amf0.cpp)
add_dependencies(mpegts project_amf0)

add_executable(mpeg_ts_tests ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(mpeg_ts_tests mpegts)

add_executable(mpeg_ts_dmx_tests ${CMAKE_CURRENT_SOURCE_DIR}/main_dmx.cpp)
target_link_libraries(mpeg_ts_dmx_tests mpegts)