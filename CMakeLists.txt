cmake_minimum_required(VERSION 3.24)

set(CMAKE_C_STANDARD           17)
set(CMAKE_CXX_STANDARD         20)
set(CMAKE_CXX_FLAGS_RELEASE    "${CMAKE_CXX_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -s")

set(LUACPP_LUA_VERSIONS        547 550)

if(DEFINED LUACPP_LUA_VERSION AND NOT ${LUACPP_LUA_VERSION} IN_LIST LUACPP_LUA_VERSIONS)
	message(FATAL_ERROR "LUACPP_LUA_VERSION must be one of the following: ${LUACPP_LUA_VERSIONS}")
endif()

project(LuaCPP)
add_library(luacpp INTERFACE)
target_include_directories(luacpp INTERFACE ${CMAKE_CURRENT_LIST_DIR})

if(DEFINED LUACPP_LUA_VERSION)
	add_subdirectory(lua${LUACPP_LUA_VERSION})
	target_link_libraries(luacpp INTERFACE lua${LUACPP_LUA_VERSION})
endif()
