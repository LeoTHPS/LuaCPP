include(FetchContent)

set(LUA550_URL      "https://www.lua.org/ftp/lua-5.5.0.tar.gz")
set(LUA550_URL_HASH SHA256=57ccc32bbbd005cab75bcc52444052535af691789dba2b9016d5c50640d68b3d)

FetchContent_Declare(lua550_external URL "${LUA550_URL}" URL_HASH ${LUA550_URL_HASH} EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(lua550_external)

add_library(lua550 STATIC
	${lua550_external_SOURCE_DIR}/src/lapi.c
	${lua550_external_SOURCE_DIR}/src/lauxlib.c
	${lua550_external_SOURCE_DIR}/src/lbaselib.c
	${lua550_external_SOURCE_DIR}/src/lcode.c
	${lua550_external_SOURCE_DIR}/src/lcorolib.c
	${lua550_external_SOURCE_DIR}/src/lctype.c
	${lua550_external_SOURCE_DIR}/src/ldblib.c
	${lua550_external_SOURCE_DIR}/src/ldebug.c
	${lua550_external_SOURCE_DIR}/src/ldo.c
	${lua550_external_SOURCE_DIR}/src/ldump.c
	${lua550_external_SOURCE_DIR}/src/lfunc.c
	${lua550_external_SOURCE_DIR}/src/lgc.c
	${lua550_external_SOURCE_DIR}/src/linit.c
	${lua550_external_SOURCE_DIR}/src/liolib.c
	${lua550_external_SOURCE_DIR}/src/llex.c
	${lua550_external_SOURCE_DIR}/src/lmathlib.c
	${lua550_external_SOURCE_DIR}/src/lmem.c
	${lua550_external_SOURCE_DIR}/src/loadlib.c
	${lua550_external_SOURCE_DIR}/src/lobject.c
	${lua550_external_SOURCE_DIR}/src/lopcodes.c
	${lua550_external_SOURCE_DIR}/src/loslib.c
	${lua550_external_SOURCE_DIR}/src/lparser.c
	${lua550_external_SOURCE_DIR}/src/lstate.c
	${lua550_external_SOURCE_DIR}/src/lstring.c
	${lua550_external_SOURCE_DIR}/src/lstrlib.c
	${lua550_external_SOURCE_DIR}/src/ltable.c
	${lua550_external_SOURCE_DIR}/src/ltablib.c
	${lua550_external_SOURCE_DIR}/src/ltm.c
	# ${lua550_external_SOURCE_DIR}/src/lua.c
	# ${lua550_external_SOURCE_DIR}/src/luac.c
	${lua550_external_SOURCE_DIR}/src/lundump.c
	${lua550_external_SOURCE_DIR}/src/lutf8lib.c
	${lua550_external_SOURCE_DIR}/src/lvm.c
	${lua550_external_SOURCE_DIR}/src/lzio.c
)
target_compile_definitions(lua550 PUBLIC -DLUA_COMPAT_5_3=1)
target_include_directories(lua550 PUBLIC ${lua550_external_SOURCE_DIR}/src/)
