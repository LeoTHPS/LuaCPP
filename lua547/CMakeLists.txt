include(FetchContent)

set(LUA547_URL      "https://www.lua.org/ftp/lua-5.4.7.tar.gz")
set(LUA547_URL_HASH SHA256=9fbf5e28ef86c69858f6d3d34eccc32e911c1a28b4120ff3e84aaa70cfbf1e30)

FetchContent_Declare(lua547_external URL "${LUA547_URL}" URL_HASH ${LUA547_URL_HASH} EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(lua547_external)

add_library(lua547 STATIC
	${lua547_external_SOURCE_DIR}/src/lapi.c
	${lua547_external_SOURCE_DIR}/src/lauxlib.c
	${lua547_external_SOURCE_DIR}/src/lbaselib.c
	${lua547_external_SOURCE_DIR}/src/lcode.c
	${lua547_external_SOURCE_DIR}/src/lcorolib.c
	${lua547_external_SOURCE_DIR}/src/lctype.c
	${lua547_external_SOURCE_DIR}/src/ldblib.c
	${lua547_external_SOURCE_DIR}/src/ldebug.c
	${lua547_external_SOURCE_DIR}/src/ldo.c
	${lua547_external_SOURCE_DIR}/src/ldump.c
	${lua547_external_SOURCE_DIR}/src/lfunc.c
	${lua547_external_SOURCE_DIR}/src/lgc.c
	${lua547_external_SOURCE_DIR}/src/linit.c
	${lua547_external_SOURCE_DIR}/src/liolib.c
	${lua547_external_SOURCE_DIR}/src/llex.c
	${lua547_external_SOURCE_DIR}/src/lmathlib.c
	${lua547_external_SOURCE_DIR}/src/lmem.c
	${lua547_external_SOURCE_DIR}/src/loadlib.c
	${lua547_external_SOURCE_DIR}/src/lobject.c
	${lua547_external_SOURCE_DIR}/src/lopcodes.c
	${lua547_external_SOURCE_DIR}/src/loslib.c
	${lua547_external_SOURCE_DIR}/src/lparser.c
	${lua547_external_SOURCE_DIR}/src/lstate.c
	${lua547_external_SOURCE_DIR}/src/lstring.c
	${lua547_external_SOURCE_DIR}/src/lstrlib.c
	${lua547_external_SOURCE_DIR}/src/ltable.c
	${lua547_external_SOURCE_DIR}/src/ltablib.c
	${lua547_external_SOURCE_DIR}/src/ltm.c
	# ${lua547_external_SOURCE_DIR}/src/lua.c
	# ${lua547_external_SOURCE_DIR}/src/luac.c
	${lua547_external_SOURCE_DIR}/src/lundump.c
	${lua547_external_SOURCE_DIR}/src/lutf8lib.c
	${lua547_external_SOURCE_DIR}/src/lvm.c
	${lua547_external_SOURCE_DIR}/src/lzio.c
)
target_compile_definitions(lua547 PUBLIC -DLUA_COMPAT_5_3=1)
target_include_directories(lua547 PUBLIC ${lua547_external_SOURCE_DIR}/src/)
