cmake_minimum_required(VERSION 3.24)

set(CMAKE_C_STANDARD           17)
set(CMAKE_CXX_STANDARD         20)
set(CMAKE_CXX_FLAGS_RELEASE    "${CMAKE_CXX_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -s")

project(LuaCPP)

add_subdirectory(lua550)

add_library(luacpp INTERFACE)
target_link_libraries(luacpp INTERFACE lua550)
target_include_directories(luacpp INTERFACE ${CMAKE_CURRENT_LIST_DIR})
