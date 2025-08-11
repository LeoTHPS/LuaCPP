#include <iostream>

#include <LuaCPP.hpp>

int main(int argc, char* argv[])
{
	LuaCPP                                                                                 lua;
	LuaCPP::Function<int(int a, int b, LuaCPP::Optional<LuaCPP::Function<int(int, int)>>)> lua_do_the_thing([&lua](int a, int b, LuaCPP::Optional<LuaCPP::Function<int(int, int)>> callback) {
		if (callback)
			return callback->Execute(a, b);

		return 0;
	});

	try
	{
		lua.LoadLibrary(LuaCPP::Libraries::All);

		lua.SetGlobal("do_the_thing", lua_do_the_thing);

		lua.RunFile("./demo.lua");
	}
	catch(const std::exception& exception)
	{
		std::cerr << exception.what() << std::endl;
	}

	return 0;
}
