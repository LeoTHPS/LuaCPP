#include <iostream>

#include <LuaCPP.hpp>

int do_the_thing(int a, int b)
{
	return a + b;
}

#define lua_set_global(lua, value)          lua_set_global_ex(lua, #value, value)
#define lua_set_global_ex(lua, name, value) lua.SetGlobal<value>(name)

int main(int argc, char* argv[])
{
	if (auto lua = LuaCPP())
	{
		lua.LoadLibrary(LuaCPP::Libraries::All);

		lua_set_global(lua, do_the_thing);

		try
		{
			if (!lua.RunFile("./demo.lua"))
				std::cerr << "demo.lua not found" << std::endl;
		}
		catch(const std::exception& exception)
		{
			std::cerr << exception.what() << std::endl;
		}
	}

	return 0;
}
