#pragma once
#include <list>
#include <tuple>
#include <memory>
#include <string>
#include <vector>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <utility>
#include <exception>
#include <filesystem>
#include <functional>
#include <type_traits>

#include <lua.hpp>

#if (LUA_VERSION_MAJOR_N == 5) && (LUA_VERSION_MINOR_N == 4)
	#define LUACPP_IS_LUA54 1
#elif (LUA_VERSION_MAJOR_N == 5) && (LUA_VERSION_MINOR_N == 5)
	#define LUACPP_IS_LUA55 1
#else
	#error Lua version not supported
#endif

class LuaCPP
{
public:
	enum class Types
	{
		None          = LUA_TNONE,
		Null          = LUA_TNIL,
		Boolean       = LUA_TBOOLEAN,
		LightUserData = LUA_TLIGHTUSERDATA,
		Number        = LUA_TNUMBER,
		String        = LUA_TSTRING,
		Table         = LUA_TTABLE,
		Function      = LUA_TFUNCTION,
		UserData      = LUA_TUSERDATA,
		Thread        = LUA_TTHREAD
	};

	enum class Libraries
	{
		All,

		Base,
		CoRoutine,
		Table,
		IO,
		OS,
		String,
		UTF8,
		Math,
		Debug,
		Package
	};

	enum class FunctionTypes
	{
		None, C, Lua
	};

	class Thread;
	template<typename T>
	class UserData;
	template<typename F>
	class Function;
	template<typename T>
	class Optional;

private:
	template<typename T>
	struct Is_Char
	{
		static constexpr bool Value = std::is_same<T, char>::value;
	};
	template<typename T>
	struct Is_Null
	{
		static constexpr bool Value = std::is_null_pointer<T>::value;
	};
	template<typename T>
	struct Is_Number
	{
		static constexpr bool Value = !std::is_same<T, bool>::value &&
			(std::is_enum<T>::value ||
			std::is_integral<T>::value ||
			std::is_floating_point<T>::value);
	};
	template<typename T>
	struct Is_Boolean
	{
		static constexpr bool Value = std::is_same<T, bool>::value;
	};
	template<typename T>
	struct Is_String
	{
		static constexpr bool Value =
			std::is_same<T, std::string>::value ||
			std::is_same<T, std::string_view>::value ||
			std::is_same<T, const char*>::value;
	};
	template<typename T>
	struct Is_CString
	{
		static constexpr bool Value = std::is_same<T, const char*>::value;
	};
	template<typename T>
	struct Is_StringView
	{
		static constexpr bool Value = std::is_same<T, std::string_view>::value;
	};
	// template<typename T>
	// struct Is_Table
	// {
	// 	static constexpr bool Value = std::is_same<T, Table>::value;
	// };
	template<typename T>
	struct Is_Tuple
	{
		static constexpr bool Value = false;
	};
	template<typename ... T>
	struct Is_Tuple<std::tuple<T ...>>
	{
		static constexpr bool Value = true;
	};
	template<typename T>
	struct Is_Function
	{
		static constexpr bool Value = false;
	};
	template<typename T>
	struct Is_Function<Function<T>>
	{
		static constexpr bool Value = true;
	};
	template<typename T>
	struct Is_CFunction
	{
		static constexpr bool Value = false;
	};
	template<typename T, typename ... TArgs>
	struct Is_CFunction<T(*)(TArgs ...)>
	{
		static constexpr bool Value = true;
	};
	template<typename T>
	struct Is_Optional
	{
		static constexpr bool Value = false;
	};
	template<typename T>
	struct Is_Optional<Optional<T>>
	{
		static constexpr bool Value = true;
	};
	template<typename T>
	struct Is_Thread
	{
		static constexpr bool Value = std::is_same<T, Thread>::value;
	};
	template<typename T>
	struct Is_UserData
	{
		static constexpr bool Value = false;
	};
	template<typename T>
	struct Is_UserData<UserData<T>>
	{
		static constexpr bool Value = true;
	};
	template<typename T>
	struct Is_LightUserData
	{
		static constexpr bool Value = std::is_pointer<T>::value;
	};

	template<typename T>
	struct Get_Type
	{
		static constexpr Types Value =
			Is_Null<T>::Value                          ? Types::Null :
			Is_Number<T>::Value                        ? Types::Number :
			Is_Boolean<T>::Value                       ? Types::Boolean :
			(Is_String<T>::Value || Is_Char<T>::Value) ? Types::String :
			// Is_Table<T>::Value                         ? Types::Table :
			Is_Function<T>::Value                      ? Types::Function :
			Is_Thread<T>::Value                        ? Types::Thread :
			Is_UserData<T>::Value                      ? Types::UserData :
			Is_LightUserData<T>::Value                 ? Types::LightUserData : Types::None;
	};

	class Exception
		: public std::exception
	{
		std::string message;

	public:
		Exception()
		{
		}
		Exception(std::string_view function, int result)
			: Exception(function, std::to_string(result))
		{
		}
		Exception(std::string_view function, lua_State* lua)
			: Exception(function, lua_tostring(lua, -1))
		{
			lua_pop(lua, 1);
		}
		Exception(std::string_view function, std::string_view message)
			: message(std::string("Error calling '").append(function).append("': ").append(message))
		{
		}

		virtual const char* what() const noexcept
		{
			return message.c_str();
		}
	};

public:
	template<typename T, typename ... TArgs>
	class Function<T(TArgs ...)>
	{
		friend LuaCPP;

		typedef std::function<T(TArgs ...)> CFunction;

		template<typename F>
		class Detour;
		template<typename T_RETURN, typename ... T_ARGS>
		class Detour<T_RETURN(T_ARGS ...)>
		{
		public:
			static constexpr int      C(lua_State* lua, const CFunction& function)
			{
				return C(lua, function, std::make_index_sequence<sizeof...(T_ARGS)> {});
			}
			template<size_t ... I>
			static constexpr int      C(lua_State* lua, const CFunction& function, std::index_sequence<I ...>)
			{
				if constexpr (std::is_same<T, void>::value)
					return function(Peek<I>(lua) ...), 0;
				else
					return Push<T_RETURN>(lua, function(Peek<I>(lua) ...));
			}

			static constexpr T_RETURN Lua(lua_State* lua, T_ARGS ... args)
			{
				if constexpr (std::is_same<T, void>::value)
					lua_call(lua, (Push<T_ARGS>(lua, args) + ...), 0);
				else
				{
					lua_call(lua, (Push<T_ARGS>(lua, args) + ...), LUA_MULTRET);

					return Pop<-1, T_RETURN>(lua);
				}
			}
			// @throw std::exception
			static constexpr T_RETURN LuaProtected(lua_State* lua, T_ARGS ... args)
			{
				if constexpr (std::is_same<T, void>::value)
				{
					if (lua_pcall(lua, (Push<T_ARGS>(lua, args) + ...), 0, 0) != LUA_OK)
						throw Exception("lua_pcall", lua);
				}
				else
				{
					if (lua_pcall(lua, (Push<T_ARGS>(lua, args) + ...), LUA_MULTRET, 0) != LUA_OK)
						throw Exception(lua, "lua_pcall");

					return Pop<-1, T_RETURN>(lua);
				}
			}
		};

		struct Context
		{
			lua_State*    lua;
			FunctionTypes type;
			CFunction     function;
			int           reference;
			bool          take_ownership;

			Context()
				: type(FunctionTypes::None)
			{
			}

			template<typename F>
			Context(F&& function)
				: type(FunctionTypes::C),
				function(std::move(function))
			{
			}

			Context(lua_State* lua, int reference, bool take_ownership)
				: lua(lua),
				type(FunctionTypes::Lua),
				reference(reference),
				take_ownership(take_ownership)
			{
			}

			~Context()
			{
				if (type == FunctionTypes::Lua)
					if (take_ownership)
						luaL_unref(lua, LUA_REGISTRYINDEX, reference);
			}
		};

		std::shared_ptr<Context> context;

	public:
		Function()
			: context(new Context())
		{
		}

		template<typename F>
		Function(F&& function)
			: context(new Context(std::move(function)))
		{
		}

		Function(Function&& function)
			: context(std::move(function.context))
		{
		}
		Function(const Function& function)
			: context(function.context)
		{
		}

		Function(lua_State* lua, int reference, bool take_ownership)
			: context(new Context(lua, reference, take_ownership))
		{
		}

		virtual ~Function()
		{
		}

		constexpr auto GetType() const
		{
			return context ? context->type : FunctionTypes::None;
		}

		constexpr auto GetCFunction() const
		{
			return context ? &context->function : nullptr;
		}

		constexpr auto GetReference() const
		{
			return context ? context->reference : 0;
		}

		constexpr auto GetReferenceCount() const
		{
			return context ? context.use_count() : 0;
		}

		// @throw std::exception
		T Execute(TArgs ... args) const
		{
			assert(context);

			switch (context->type)
			{
				case FunctionTypes::C:
					return context->function(std::forward<TArgs>(args) ...);

				case FunctionTypes::Lua:
					if (int type = lua_rawgeti(context->lua, LUA_REGISTRYINDEX, context->reference); type != LUA_TFUNCTION)
						throw Exception("lua_rawgeti", type);
					return ExecuteLua(context->lua, std::forward<TArgs>(args) ...);
			}

			throw Exception("LuaCPP::Function::Execute", "context->type == FunctionTypes::None");
		}

		// @throw std::exception
		T ExecuteProtected(TArgs ... args) const
		{
			assert(context);

			switch (context->type)
			{
				case FunctionTypes::C:
					return context->function(std::forward<TArgs>(args) ...);

				case FunctionTypes::Lua:
					if (int type = lua_rawgeti(context->lua, LUA_REGISTRYINDEX, context->reference); type != LUA_TFUNCTION)
						throw Exception("lua_rawgeti", type);
					return ExecuteLuaProtected(context->lua, std::forward<TArgs>(args) ...);
			}

			throw Exception("LuaCPP::Function::ExecuteProtected", "context->type == FunctionTypes::None");
		}

		void Release()
		{
			context.reset();
		}

		constexpr operator bool() const
		{
			if (context)
				switch (context->type)
				{
					case FunctionTypes::C:   return context->function.operator bool();
					case FunctionTypes::Lua: return context->lua != nullptr;
				}

			return false;
		}

		auto& operator = (Function&& function)
		{
			context = std::move(function.context);

			return *this;
		}
		auto& operator = (const Function& function)
		{
			context = function.context;

			return *this;
		}

		constexpr bool operator == (const Function& function) const
		{
			return context == function.context;
		}
		constexpr bool operator != (const Function& function) const
		{
			return !operator==(function);
		}

	private:
		template<int I, typename TYPE>
		static auto Pop(lua_State* lua)
		{
			TYPE value;

			if (!LuaCPP::Pop(lua, value))
			{
				if constexpr (I == -1)
					lua_pushstring(lua, "Error popping return value");
				else
					lua_pushfstring(lua, "Error popping arg #%i", 1 + I);

				lua_error(lua);
			}

			return value;
		}
		template<size_t I>
		static auto Peek(lua_State* lua)
		{
			typename std::tuple_element<I, std::tuple<TArgs ...>>::type value;

			if (!LuaCPP::Peek(lua, 1 + I, value))
			{
				lua_pushfstring(lua, "Error peeking arg #%s", std::to_string(1 + I).c_str());
				lua_error(lua);
			}

			return value;
		}

	private:
		static constexpr int ExecuteC(lua_State* lua)
		{
			return Detour<T(TArgs ...)>::C(lua, reinterpret_cast<const Context*>(lua_touserdata(lua, lua_upvalueindex(1)))->function);
		}
		static constexpr T   ExecuteLua(lua_State* lua, TArgs ... args)
		{
			return Detour<T(TArgs ...)>::Lua(lua, std::forward<TArgs>(args) ...);
		}
		static constexpr T   ExecuteLuaProtected(lua_State* lua, TArgs ... args)
		{
			return Detour<T(TArgs ...)>::LuaProtected(lua, std::forward<TArgs>(args) ...);
		}
	};

	template<typename T>
	class Optional
	{
		friend LuaCPP;

		static_assert(Get_Type<T>::Value != Types::None);

		bool       is_set = false;

		T          value;

	public:
		Optional()
		{
		}

		Optional(Optional&& optional)
			: is_set(optional.is_set),
			value(std::move(optional.value))
		{
			optional.is_set = false;
		}
		Optional(const Optional& optional)
			: is_set(optional.is_set),
			value(optional.value)
		{
		}

		virtual ~Optional()
		{
		}

		constexpr operator bool () const
		{
			return is_set;
		}

		constexpr const T& operator * () const
		{
			return value;
		}

		constexpr const T* operator -> () const
		{
			return is_set ? &value : nullptr;
		}
	};

	template<auto F>
	class CFunction
	{
		template<typename>
		class Detour;
		template<typename T, typename ... TArgs>
		class Detour<T(*)(TArgs ...)>
		{
			Detour() = delete;

		public:
			static constexpr int Execute(lua_State* lua)
			{
				return Execute(lua, std::make_index_sequence<sizeof...(TArgs)> {});
			}
			template<size_t ... I>
			static constexpr int Execute(lua_State* lua, std::index_sequence<I ...>)
			{
				if constexpr (std::is_same<T, void>::value)
					return F(Peek<I>(lua) ...), 0;
				else
					return Push(lua, F(Peek<I>(lua) ...));
			}

		private:
			template<size_t I>
			static constexpr auto Peek(lua_State* lua)
			{
				typename std::tuple_element<I, std::tuple<TArgs ...>>::type value;

				if (!LuaCPP::Peek(lua, 1 + I, value))
				{
					lua_pushfstring(lua, "Error peeking arg #%s", std::to_string(1 + I).c_str());
					lua_error(lua);
				}

				return value;
			}
		};

		CFunction() = delete;

	public:
		static constexpr int Execute(lua_State* lua)
		{
			return Detour<decltype(F)>::Execute(lua);
		}
	};

private:
	lua_State* lua;
	bool       lua_is_owned;

	LuaCPP(const LuaCPP&) = delete;

public:
	LuaCPP()
		: lua(luaL_newstate()),
		lua_is_owned(true)
	{
	}
	LuaCPP(LuaCPP&& state)
		: lua(state.lua),
		lua_is_owned(state.lua_is_owned)
	{
		state.lua          = nullptr;
		state.lua_is_owned = false;
	}

	LuaCPP(lua_Alloc alloc, void* param)
		: lua(lua_newstate(alloc, param, luaL_makeseed(nullptr))),
		lua_is_owned(true)
	{
	}

	LuaCPP(lua_State* state, bool take_ownership)
		: lua(state),
		lua_is_owned(take_ownership)
	{
	}

	virtual ~LuaCPP()
	{
		Release();
	}

	// @throw std::exception
	void Run(std::string_view lua)
	{
		assert(this->lua != nullptr);

		if (luaL_dostring(this->lua, lua.data()) != LUA_OK)
			throw Exception("luaL_dostring", this->lua);
	}
	// @throw std::exception
	void Run(const void* buffer, size_t size, std::string_view name)
	{
		assert(lua != nullptr);

		if (luaL_loadbuffer(lua, (const char*)buffer, size, name.data()) != LUA_OK)
			throw Exception("luaL_loadbuffer", lua);

		if (lua_pcall(lua, 0, 0, 0) != LUA_OK)
		{
			lua_remove(lua, -2);

			throw Exception("lua_pcall", lua);
		}
	}
	// @throw std::exception
	// @return false if not found
	bool RunFile(std::string_view path)
	{
		assert(lua != nullptr);

		if (!FileExists(path))
			return false;

		if (luaL_dofile(lua, path.data()) != LUA_OK)
			throw Exception("luaL_dofile", lua);

		return true;
	}

	void LoadLibrary(Libraries value)
	{
		assert(lua != nullptr);

		switch (value)
		{
			case Libraries::All:       luaL_openlibs(lua);                                         break;
			case Libraries::Base:      luaL_requiref(lua, "_G",            &luaopen_base, 1);      break;
			case Libraries::CoRoutine: luaL_requiref(lua, LUA_COLIBNAME,   &luaopen_coroutine, 1); break;
			case Libraries::Table:     luaL_requiref(lua, LUA_TABLIBNAME,  &luaopen_table, 1);     break;
			case Libraries::IO:        luaL_requiref(lua, LUA_IOLIBNAME,   &luaopen_io, 1);        break;
			case Libraries::OS:        luaL_requiref(lua, LUA_OSLIBNAME,   &luaopen_os, 1);        break;
			case Libraries::String:    luaL_requiref(lua, LUA_STRLIBNAME,  &luaopen_string, 1);    break;
			case Libraries::UTF8:      luaL_requiref(lua, LUA_UTF8LIBNAME, &luaopen_utf8, 1);      break;
			case Libraries::Math:      luaL_requiref(lua, LUA_MATHLIBNAME, &luaopen_math, 1);      break;
			case Libraries::Debug:     luaL_requiref(lua, LUA_DBLIBNAME,   &luaopen_debug, 1);     break;
			case Libraries::Package:   luaL_requiref(lua, LUA_LOADLIBNAME, &luaopen_package, 1);   break;
		}
	}

	// @throw std::exception
	// @return false if not found
	bool Compile(std::string_view lua, std::vector<uint8_t>& buffer, bool include_debug_information)
	{
		assert(this->lua != nullptr);

		if (luaL_loadstring(this->lua, lua.data()) != LUA_OK)
			throw Exception("luaL_loadstring", this->lua);

		auto writer = [](lua_State* lua, const void* buffer, size_t size, void* param)->int
		{
			((std::vector<uint8_t>*)param)->resize(size);
			memcpy(((std::vector<uint8_t>*)param)->data(), buffer, size);

			return LUA_OK;
		};

		int result;

		if ((result = lua_dump(this->lua, writer, &buffer, include_debug_information ? 0 : 1)) != LUA_OK)
		{
			lua_pop(this->lua, 1);

			throw Exception("lua_dump", result);
		}

		return true;
	}
	// @throw std::exception
	// @return false if not found
	bool Compile(std::string_view lua, std::string_view destination, bool include_debug_information)
	{
		assert(this->lua != nullptr);

		if (luaL_loadstring(this->lua, lua.data()) != LUA_OK)
			throw Exception("luaL_loadstring", this->lua);

		struct Context
		{
			std::ofstream stream;
			Exception     exception;
			bool          exception_is_set;
		};

		Context context = {};

		context.stream.exceptions(std::ios::failbit | std::ios::badbit);

		try
		{
			context.stream.open(destination.data(), std::ios::out | std::ios::trunc | std::ios::binary);
		}
		catch (const std::exception& exception)
		{
			lua_pop(this->lua, 1);

			throw Exception("std::ofstream::open", exception.what());
		}

		auto writer = [](lua_State* lua, const void* buffer, size_t size, void* param)->int
		{
			auto context = (Context*)param;

			try
			{
				context->stream.write((const char*)buffer, size);
			}
			catch (const std::exception& exception)
			{
				context->exception        = Exception("std::ofstream::write", exception.what());
				context->exception_is_set = true;

				return LUA_ERRERR;
			}

			return LUA_OK;
		};

		int result;

		if ((result = lua_dump(this->lua, writer, &context, include_debug_information ? 0 : 1)) != LUA_OK)
		{
			lua_pop(this->lua, 1);

			throw Exception("lua_dump", result);
		}

		return true;
	}
	// @throw std::exception
	// @return false if not found
	bool CompileFile(std::string_view source, std::vector<uint8_t>& buffer, bool include_debug_information)
	{
		assert(lua != nullptr);

		if (!FileExists(source))
			return false;

		if (luaL_loadfile(this->lua, source.data()) != LUA_OK)
			throw Exception("luaL_loadfile", this->lua);

		auto writer = [](lua_State* lua, const void* buffer, size_t size, void* param)->int
		{
			((std::vector<uint8_t>*)param)->resize(size);
			memcpy(((std::vector<uint8_t>*)param)->data(), buffer, size);

			return LUA_OK;
		};

		int result;

		if ((result = lua_dump(lua, writer, &buffer, include_debug_information ? 0 : 1)) != LUA_OK)
		{
			lua_pop(lua, 1);

			throw Exception("lua_dump", result);
		}

		return true;
	}
	// @throw std::exception
	// @return false if not found
	bool CompileFile(std::string_view source, std::string_view destination, bool include_debug_information)
	{
		assert(lua != nullptr);

		if (!FileExists(source))
			return false;

		if (luaL_loadfile(lua, source.data()) != LUA_OK)
			throw Exception("luaL_loadfile", lua);

		struct Context
		{
			std::ofstream stream;
			Exception     exception;
			bool          exception_is_set;
		};

		Context context = {};

		context.stream.exceptions(std::ios::failbit | std::ios::badbit);

		try
		{
			context.stream.open(destination.data(), std::ios::out | std::ios::trunc | std::ios::binary);
		}
		catch (const std::exception& exception)
		{
			lua_pop(lua, 1);

			throw Exception("std::ofstream::open", exception.what());
		}

		auto writer = [](lua_State* lua, const void* buffer, size_t size, void* param)->int
		{
			auto context = (Context*)param;

			try
			{
				context->stream.write((const char*)buffer, size);
			}
			catch (const std::exception& exception)
			{
				context->exception        = Exception("std::ofstream::write", exception.what());
				context->exception_is_set = true;

				return LUA_ERRERR;
			}

			return LUA_OK;
		};

		int result;

		if ((result = lua_dump(lua, writer, &context, include_debug_information ? 0 : 1)) != LUA_OK)
		{
			lua_pop(lua, 1);

			throw Exception("lua_dump", result);
		}

		return true;
	}

	// @return 0 on not found
	// @return -1 on invalid type
	template<typename T>
	int  GetGlobal(std::string_view name, T& value) const
	{
		assert(lua != nullptr);

		static_assert(Get_Type<T>::Value != Types::None);

		auto type = lua_getglobal(lua, name.data());

		if (type == LUA_TNONE)
			return 0;

		if (type != static_cast<int>(Get_Type<T>::Value))
		{
			Pop(lua);

			return -1;
		}

		return Pop<T>(lua, value) ? 1 : 0;
	}

	auto GetGlobalType(std::string_view name) const
	{
		assert(lua != nullptr);

		auto type = lua_getglobal(lua, name.data());

		if (type != LUA_TNONE)
			Pop(lua);

		return static_cast<Types>(type);
	}

	template<auto VALUE>
	void SetGlobal(std::string_view name)
	{
		assert(lua != nullptr);

		if constexpr (Is_CFunction<decltype(VALUE)>::Value)
		{
			lua_pushcclosure(lua, &CFunction<VALUE>::Execute, 0);
			lua_setglobal(lua, name.data());
		}
		else
			return SetGlobal(name, VALUE);
	}
	template<typename T>
	void SetGlobal(std::string_view name, const T& value)
	{
		assert(lua != nullptr);

		static_assert(!Is_CFunction<T>::Value);
		static_assert(Get_Type<T>::Value != Types::None);

		Push<T>(lua, value);
		lua_setglobal(lua, name.data());
	}

	void RemoveGlobal(std::string_view name)
	{
		assert(lua != nullptr);

		lua_pushnil(lua);
		lua_setglobal(lua, name.data());
	}

	void Release()
	{
		if (lua)
		{
			if (lua_is_owned)
				lua_close(lua);

			lua          = nullptr;
			lua_is_owned = false;
		}
	}

	constexpr operator bool() const
	{
		return lua != nullptr;
	}

	constexpr operator lua_State*() const
	{
		return lua;
	}

	auto& operator = (LuaCPP&& state)
	{
		if (lua && lua_is_owned)
			lua_close(lua);

		lua = state.lua;
		state.lua = nullptr;

		lua_is_owned = state.lua_is_owned;
		state.lua_is_owned = false;

		return *this;
	}

	constexpr bool operator == (const LuaCPP& state) const
	{
		return lua == state.lua;
	}
	constexpr bool operator != (const LuaCPP& state) const
	{
		return !operator==(state);
	}

private:
	template<typename T>
	static           bool Pop(lua_State* lua, T& value)
	{
		static_assert(Get_Type<T>::Value != Types::None);

		if (lua_gettop(lua) == 0)
			return false;

		if (!Peek(lua, 1, value))
			return false;

		lua_pop(lua, 1);

		return true;
	}
	static           void Pop(lua_State* lua, size_t size = 1)
	{
		lua_pop(lua, static_cast<int>(size));
	}
	template<typename F>
	static           bool Pop(lua_State* lua, Function<F>& value)
	{
		value = Function<F>(lua, luaL_ref(lua, LUA_REGISTRYINDEX));

		return true;
	}
	template<typename T>
	static constexpr bool Pop(lua_State* lua, Optional<T>& value)
	{
		value.is_set = Pop(lua, value.value);

		return true;
	}
	template<typename ... T>
	static constexpr bool Pop(lua_State* lua, std::tuple<T ...>& value)
	{
		return Pop(lua, value, std::make_index_sequence<sizeof...(T)> {});
	}
	template<size_t ... I, typename ... T>
	static constexpr bool Pop(lua_State* lua, std::tuple<T ...>& value, std::index_sequence<I ...>)
	{
		return (Pop<typename std::tuple_element<I, std::tuple<T ...>>::type>(lua, std::get<I>(value)) && ...);
	}

	template<typename T>
	static           bool Peek(lua_State* lua, size_t index, T& value)
	{
		if (index > lua_gettop(lua))
			return false;

		if constexpr (Is_Char<T>::Value)
		{
			if (auto string = lua_tolstring(lua, static_cast<int>(index), nullptr))
			{
				value = string[0];

				return true;
			}
		}
		else if constexpr (Is_Number<T>::Value)
		{
			if constexpr (std::is_floating_point<T>::value)
				value = static_cast<T>(lua_tonumber(lua, static_cast<int>(index)));
			else
				value = static_cast<T>(lua_tointeger(lua, static_cast<int>(index)));

			return true;
		}
		else if constexpr (Is_Boolean<T>::Value)
		{
			value = lua_toboolean(lua, static_cast<int>(index)) ? true : false;

			return true;
		}
		else if constexpr (Is_String<T>::Value)
		{
			if constexpr (Is_CString<T>::Value)
			{
				value = lua_tostring(lua, static_cast<int>(index));

				return true;
			}
			else if constexpr (Is_StringView<T>::Value)
			{
				if (size_t length; auto string = lua_tolstring(lua, static_cast<int>(index), &length))
					value = std::string_view(string, length);

				return true;
			}
			else
			{
				if (size_t length; auto string = lua_tolstring(lua, static_cast<int>(index), &length))
					value = std::string(string, length);

				return true;
			}
		}
		else if constexpr (Is_Thread<T>::Value)
		{
			// TODO: implement
		}
		else if constexpr (Is_UserData<T>::Value)
		{
			// TODO: implement
		}
		else if constexpr (Is_LightUserData<T>::Value)
		{
			if (auto data = lua_touserdata(lua, static_cast<int>(index)))
			{
				value = reinterpret_cast<T>(data);

				return true;
			}
			else if (lua_isnil(lua, static_cast<int>(index)))
			{
				value = nullptr;

				return true;
			}
		}

		return false;
	}
	template<typename F>
	static           bool Peek(lua_State* lua, size_t index, Function<F>& value)
	{
		lua_pushvalue(lua, static_cast<int>(index));

		int reference;

		if ((reference = luaL_ref(lua, LUA_REGISTRYINDEX)) == LUA_REFNIL)
		{
			lua_pop(lua, 1);

			return false;
		}

		value = Function<F>(lua, reference, true);

		return true;
	}
	template<typename T>
	static constexpr bool Peek(lua_State* lua, size_t index, Optional<T>& value)
	{
		value.is_set = Peek(lua, index, value.value);

		return true;
	}
	template<typename ... T>
	static constexpr bool Peek(lua_State* lua, size_t index, std::tuple<T ...>& value)
	{
		return Peek(lua, index, value, std::make_index_sequence<sizeof...(T)> {});
	}
	template<typename ... T, size_t ... I>
	static constexpr bool Peek(lua_State* lua, size_t index, std::tuple<T ...>& value, std::index_sequence<I ...>)
	{
		return (Peek<typename std::tuple_element<I, std::tuple<T ...>>::type>(lua, index + I, std::get<I>(value)) && ...);
	}

	template<typename T>
	static           int  Push(lua_State* lua, const T& value)
	{
		if constexpr (Is_Char<T>::Value)
		{
			lua_pushlstring(lua, &value, 1);

			return 1;
		}
		else if constexpr (Is_Number<T>::Value)
		{
			if constexpr (std::is_floating_point<T>::value)
				lua_pushnumber(lua, static_cast<lua_Number>(value));
			else
				lua_pushinteger(lua, static_cast<lua_Integer>(value));

			return 1;
		}
		else if constexpr (Is_Boolean<T>::Value)
		{
			lua_pushboolean(lua, value ? 1 : 0);

			return 1;
		}
		else if constexpr (Is_String<T>::Value)
		{
			if constexpr (Is_CString<T>::Value)
			{
				if (value == nullptr)
					lua_pushnil(lua);
				else
				{
#if defined(LUACPP_IS_LUA54)
					lua_pushstring(lua, value);
#elif defined(LUACPP_IS_LUA55)
					lua_pushexternalstring(lua, value, strlen(value), nullptr, nullptr);
#endif
				}
			}
			else if constexpr (Is_StringView<T>::Value)
			{
				if (value.data() == nullptr)
					lua_pushnil(lua);
				else
#if defined(LUACPP_IS_LUA54)
					lua_pushlstring(lua, value.data(), value.length());
#elif defined(LUACPP_IS_LUA55)
					lua_pushexternalstring(lua, value.data(), value.length(), nullptr, nullptr);
#endif
			}
			else
				lua_pushlstring(lua, value.c_str(), value.length());

			return 1;
		}
		else if constexpr (Is_Thread<T>::Value)
		{
			// TODO: implement
		}
		else if constexpr (Is_UserData<T>::Value)
		{
			// TODO: implement
		}
		else if constexpr (Is_LightUserData<T>::Value)
		{
			if (value == nullptr)
				lua_pushnil(lua);
			else
				lua_pushlightuserdata(lua, const_cast<void*>(static_cast<const void*>(value)));

			return 1;
		}

		return 0;
	}
	template<typename F>
	static           int  Push(lua_State* lua, const Function<F>& value)
	{
		switch (value.GetType())
		{
			case FunctionTypes::None:
				lua_pushnil(lua);
				break;

			case FunctionTypes::C:
				lua_pushlightuserdata(lua, value.context.get());
				lua_pushcclosure(lua, &Function<F>::ExecuteC, 1);
				break;

			case FunctionTypes::Lua:
				if (int value_type = lua_rawgeti(lua, LUA_REGISTRYINDEX, value.GetReference()); value_type != LUA_TFUNCTION)
					throw Exception("lua_rawgeti", value_type);
				break;
		}

		return 1;
	}
	template<typename T>
	static constexpr int  Push(lua_State* lua, const Optional<T>& value)
	{
		return value ? Push<T>(lua, value.value) : 0;
	}
	template<typename ... T>
	static constexpr int  Push(lua_State* lua, const std::tuple<T ...>& value)
	{
		return Push<T ...>(lua, value, std::make_index_sequence<sizeof...(T)> {});
	}
	template<typename ... T, size_t ... I>
	static constexpr int  Push(lua_State* lua, const std::tuple<T ...>& value, std::index_sequence<I ...>)
	{
		return (Push<T>(lua, std::get<I>(value)) + ...);
	}

private:
	// @throw std::exception
	static bool FileExists(std::string_view path)
	{
		if (!std::filesystem::exists(path))
			return false;

		if (!std::filesystem::is_regular_file(path) && !std::filesystem::is_symlink(path))
			return false;

		return true;
	}
};
