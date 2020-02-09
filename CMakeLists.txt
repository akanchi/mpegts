cmake_minimum_required(VERSION 3.10)
project(mpegts)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

file(GLOB libfiles "${CMAKE_CURRENT_SOURCE_DIR}/mpegts/*.cpp")
add_library(mpegts STATIC ${libfiles})

add_executable(mpegtstests ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(mpegtstests mpegts)