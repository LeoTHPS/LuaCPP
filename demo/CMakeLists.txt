cmake_minimum_required(VERSION 3.24)

set(CMAKE_C_STANDARD               17)
set(CMAKE_CXX_STANDARD             20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

set(LUACPP_LUA_VERSION             550)

add_subdirectory($ENV{LUACPP_PATH} LuaCPP)

project(demo)
add_executable(demo demo.cpp)
target_link_libraries(demo luacpp)
