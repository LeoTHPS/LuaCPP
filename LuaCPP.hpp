#pragma once
#include <list>
#include <tuple>
#include <memory>
#include <string>
#include <cassert>
#include <cstdint>
#include <utility>
#include <exception>
#include <filesystem>
#include <functional>
#include <type_traits>

#include <lua.hpp>

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

	class Table;
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
		static constexpr bool Value =
			std::is_enum<T>::value ||
			std::is_integral<T>::value ||
			std::is_floating_point<T>::value;
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
	template<typename T>
	struct Is_Table
	{
		static constexpr bool Value = std::is_same<T, Table>::value;
	};
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
			Is_Table<T>::Value                         ? Types::Table :
			Is_Function<T>::Value                      ? Types::Function :
			Is_Thread<T>::Value                        ? Types::Thread :
			Is_UserData<T>::Value                      ? Types::UserData :
			Is_LightUserData<T>::Value                 ? Types::LightUserData : Types::None;
	};

	struct Data
	{
		Types         type;
		std::string   string;
		FunctionTypes function_type;

		union
		{
			Table*        table;
			lua_Number    number;
			int           boolean;
			lua_Integer   integer;
			lua_CFunction function_c;
			int           function_lua;
			void*         light_user_data;
		};

		template<typename T>
		explicit Data(const T& value)
			: type(Get_Type<T>::Value),
			function_type(FunctionTypes::None)
		{
			static constexpr Types TYPE = Get_Type<T>::Value;

			if constexpr (TYPE == Types::Boolean)
				boolean = value;
			else if constexpr (TYPE == Types::LightUserData)
				light_user_data = value;
			else if constexpr (TYPE == Types::Number)
				number = value;
			else if constexpr (TYPE == Types::String)
				string = value;
			else if constexpr (TYPE == Types::Table)
				table = &value;
			else if constexpr (TYPE == Types::Function)
			{
				function_type = value.GetType();

				switch (function_type)
				{
					case FunctionTypes::C:
						function_c = value.context->function;
						break;

					case FunctionTypes::Lua:
						function_lua = value.context->reference;
						break;
				}
			}
			else if constexpr (TYPE == Types::UserData)
				; // TODO: implement
			else if constexpr (TYPE == Types::Thread)
				; // TODO: implement
		}
	};

	class Exception
		: public std::exception
	{
		std::string message;

	public:
		Exception(std::string&& function, lua_State* lua)
			: Exception(std::move(function), lua_tostring(lua, -1))
		{
		}

		Exception(std::string&& function, std::string&& message)
			: message(message + " [Function: " + function + "]")
		{
		}

		Exception(std::string&& file, size_t line, std::string&& message)
			: message(message + " [File: " + file + ", Line: " + std::to_string(line) + "]")
		{
		}

		virtual const char* what() const noexcept
		{
			return message.c_str();
		}
	};

public:
	/*
	class Table
	{
		friend LuaCPP;

		struct Field
		{
			Data key;
			Data value;

			template<typename T_KEY, typename T_VALUE>
			Field(const T_KEY& key, const T_VALUE& value)
				: key(key),
				value(value)
			{
			}
		};

		struct Context
		{
			lua_State*       lua;
			std::list<Field> fields;
		};

		std::shared_ptr<Context> context;

	public:
		Table()
			: context(new Context())
		{
		}

		Table(Table&& table)
			: context(std::move(table.context))
		{
		}
		Table(const Table& table)
			: context(table.context)
		{
		}

		virtual ~Table()
		{
		}

		auto GetCount() const
		{
			return context ? context->fields.size() : 0;
		}

		// @throw std::exception
		// @return 0 on not found
		// @return -1 on invalid type
		template<typename T>
		int  At(size_t index, T& value)
		{
			assert(context->lua != nullptr);

			// TODO: implement

			return 0;
		}

		// @throw std::exception
		// @return 0 on not found
		// @return -1 on invalid type
		template<typename T_KEY, typename T_VALUE>
		int  Get(const T_KEY& key, T_VALUE& value)
		{
			assert(context->lua != nullptr);

			// TODO: implement

			return 0;
		}

		// @throw std::exception
		template<typename T>
		auto GetType(const T& key) const
		{
			assert(context->lua != nullptr);

			// TODO: implement

			return Types::None;
		}

		// @throw std::exception
		auto GetTypeAt(size_t index) const
		{
			assert(context->lua != nullptr);

			for (auto& field : context->fields)
				if (!index--)
					return field.value.type;

			return Types::None;
		}

		// @throw std::exception
		template<typename T_KEY, typename T_VALUE>
		void Set(const T_KEY& key, const T_VALUE& value)
		{
			assert(context->lua != nullptr);

			// TODO: implement
		}

		void Release()
		{
			context.reset();
		}

		auto& operator = (Table&& table)
		{
			context = std::move(table.context);

			return *this;
		}
		auto& operator = (const Table& table)
		{
			context = table.context;

			return *this;
		}
	};
	*/

	template<typename T, typename ... TArgs>
	class Function<T(TArgs ...)>
	{
		friend Data;
		friend LuaCPP;

		typedef std::function<T(TArgs ...)> CFunction;

		template<typename F>
		class Detour;
		template<typename ... T_ARGS>
		class Detour<void(T_ARGS ...)>
		{
		public:
			static constexpr int  C(lua_State* lua, const CFunction& function)
			{
				return C(lua, function, std::make_index_sequence<sizeof...(T_ARGS)> {});
			}
			template<size_t ... I>
			static constexpr int  C(lua_State* lua, const CFunction& function, std::index_sequence<I ...>)
			{
				function(Peek<I>(lua) ...);

				return 0;
			}

			static constexpr void Lua(lua_State* lua, T_ARGS ... args)
			{
				lua_call(lua, (Push<T_ARGS>(lua, args) + ...), 0);
			}
			static constexpr void LuaProtected(lua_State* lua, T_ARGS ... args)
			{
				if (lua_pcall(lua, (Push<T_ARGS>(lua, args) + ...), 0, 0) != LUA_OK)
					throw Exception("lua_pcall", lua);
			}
		};
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
				return Push<T_RETURN>(lua, function(Peek<I>(lua) ...));
			}

			static constexpr T_RETURN Lua(lua_State* lua, T_ARGS ... args)
			{
				lua_call(lua, (Push<T_ARGS>(lua, args) + ...), LUA_MULTRET);

				return Pop<-1, T_RETURN>(lua);
			}
			static constexpr T_RETURN LuaProtected(lua_State* lua, T_ARGS ... args)
			{
				if (lua_pcall(lua, (Push<T_ARGS>(lua, args) + ...), LUA_MULTRET, 0) != LUA_OK)
					throw Exception(lua, "lua_pcall");

				return Pop<T_RETURN>(lua);
			}
		};

		struct Context
		{
			lua_State*    lua;
			FunctionTypes type;
			CFunction     function;
			int           reference;

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

			Context(lua_State* lua, int reference)
				: lua(lua),
				type(FunctionTypes::Lua),
				reference(reference)
			{
			}

			~Context()
			{
				if (type == FunctionTypes::Lua)
					luaL_unref(lua, LUA_REGISTRYINDEX, reference);
			}
		};

		std::shared_ptr<Context> context;

		Function(lua_State* lua, int reference)
			: context(new Context(lua, reference))
		{
		}

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

		virtual ~Function()
		{
		}

		constexpr auto GetType() const
		{
			return context ? context->type : FunctionTypes::None;
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
					if (auto type = lua_rawgeti(context->lua, LUA_REGISTRYINDEX, context->reference); type != LUA_TFUNCTION)
						throw Exception("LuaCPP::Function::Execute", "lua_rawgeti returned " + std::to_string(type));
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
					if (auto type = lua_rawgeti(context->lua, LUA_REGISTRYINDEX, context->reference); type != LUA_TFUNCTION)
						throw Exception("LuaCPP::Function::Execute", "lua_rawgeti returned " + std::to_string(type));
					return ExecuteLuaProtected(context->lua, std::forward<TArgs>(args) ...);
			}

			throw Exception("LuaCPP::Function::ExecuteProtected", "context->type == FunctionTypes::None");
		}

		void Release()
		{
			context.reset();
		}

		operator bool() const
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

private:
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
		};

		CFunction() = delete;

	public:
		static constexpr int Execute(lua_State* lua)
		{
			return Detour<decltype(F)>::Execute(lua);
		}
	};

	lua_State* lua;

	LuaCPP(const LuaCPP&) = delete;

public:
	LuaCPP()
		: lua(luaL_newstate())
	{
	}

	LuaCPP(LuaCPP&& state)
		: lua(state.lua)
	{
		state.lua = nullptr;
	}

	virtual ~LuaCPP()
	{
		Release();
	}

	constexpr auto GetHandle() const
	{
		return lua;
	}

	// @throw std::exception
	void Run(const std::string_view& lua)
	{
		assert(GetHandle() != nullptr);

		if (luaL_dostring(GetHandle(), lua.data()) != LUA_OK)
			throw Exception("luaL_dostring", GetHandle());
	}
	// @throw std::exception
	// @return false if not found
	bool RunFile(const std::string_view& path)
	{
		assert(GetHandle() != nullptr);

		if (!FileExists(path))
			return false;

		if (luaL_dofile(GetHandle(), path.data()) != LUA_OK)
			throw Exception("luaL_dofile", GetHandle());

		return true;
	}

	// @throw std::exception
	void LoadLibrary(Libraries value)
	{
		assert(GetHandle() != nullptr);

		switch (value)
		{
			case Libraries::All:       luaL_openlibs(GetHandle());                                         break;
			case Libraries::Base:      luaL_requiref(GetHandle(), "_G",            &luaopen_base, 1);      break;
			case Libraries::CoRoutine: luaL_requiref(GetHandle(), LUA_COLIBNAME,   &luaopen_coroutine, 1); break;
			case Libraries::Table:     luaL_requiref(GetHandle(), LUA_TABLIBNAME,  &luaopen_table, 1);     break;
			case Libraries::IO:        luaL_requiref(GetHandle(), LUA_IOLIBNAME,   &luaopen_io, 1);        break;
			case Libraries::OS:        luaL_requiref(GetHandle(), LUA_OSLIBNAME,   &luaopen_os, 1);        break;
			case Libraries::String:    luaL_requiref(GetHandle(), LUA_STRLIBNAME,  &luaopen_string, 1);    break;
			case Libraries::UTF8:      luaL_requiref(GetHandle(), LUA_UTF8LIBNAME, &luaopen_utf8, 1);      break;
			case Libraries::Math:      luaL_requiref(GetHandle(), LUA_MATHLIBNAME, &luaopen_math, 1);      break;
			case Libraries::Debug:     luaL_requiref(GetHandle(), LUA_DBLIBNAME,   &luaopen_debug, 1);     break;
			case Libraries::Package:   luaL_requiref(GetHandle(), LUA_LOADLIBNAME, &luaopen_package, 1);   break;
		}
	}

	// @throw std::exception
	// @return 0 on not found
	// @return -1 on invalid type
	template<typename T>
	int  GetGlobal(const std::string_view& name, T& value)
	{
		assert(GetHandle() != nullptr);

		static_assert(Get_Type<T>::Value != Types::None);
		static_assert(Get_Type<T>::Value != Types::Function);

		auto type = lua_getglobal(GetHandle(), name.data());

		if (type == LUA_TNONE)
			return 0;

		if (type != static_cast<int>(Get_Type<T>::Value))
		{
			Pop(GetHandle());

			return -1;
		}

		value = Pop<T>(GetHandle());

		return 1;
	}
	// @throw std::exception
	// @return 0 on not found
	// @return -1 on invalid type
	template<typename T, typename ... TArgs>
	int  GetGlobal(const std::string_view& name, Function<T(TArgs ...)>& value)
	{
		assert(GetHandle() != nullptr);

		auto type = lua_getglobal(GetHandle(), name.data());

		if (type == LUA_TNONE)
			return 0;

		if (type != LUA_TFUNCTION)
		{
			Pop(GetHandle());

			return -1;
		}

		lua_pushvalue(GetHandle(), 1);

		value = Function<T(TArgs ...)>(GetHandle(), luaL_ref(GetHandle(), LUA_REGISTRYINDEX));

		return 1;
	}

	// @throw std::exception
	auto GetGlobalType(const std::string_view& name)
	{
		assert(GetHandle() != nullptr);

		auto type = lua_getglobal(GetHandle(), name.data());

		if (type != LUA_TNONE)
			Pop(GetHandle());

		return static_cast<Types>(type);
	}

	// @throw std::exception
	template<auto F>
	void SetGlobal(const std::string_view& name)
	{
		assert(GetHandle() != nullptr);

		if constexpr (Is_CFunction<decltype(F)>::Value)
		{
			lua_pushcclosure(GetHandle(), &CFunction<F>::Execute, 0);
			lua_setglobal(GetHandle(), name.data());

			return;
		}
		else
			return SetGlobal(name, F);
	}
	// @throw std::exception
	template<typename T>
	void SetGlobal(const std::string_view& name, const T& value)
	{
		assert(GetHandle() != nullptr);

		static_assert(!Is_CFunction<T>::Value);
		static_assert(Get_Type<T>::Value != Types::None);

		Push<T>(GetHandle(), value);
		lua_setglobal(GetHandle(), name.data());
	}

	// @throw std::exception
	void RemoveGlobal(const std::string_view& name)
	{
		assert(GetHandle() != nullptr);

		lua_pushnil(GetHandle());
		lua_setglobal(GetHandle(), name.data());
	}

	void Release()
	{
		if (auto lua = GetHandle())
		{
			lua_close(lua);

			this->lua = nullptr;
		}
	}

	constexpr operator bool() const
	{
		return GetHandle() != nullptr;
	}

	auto& operator = (LuaCPP&& state)
	{
		if (auto lua = GetHandle())
			lua_close(lua);

		lua = state.GetHandle();
		state.lua = nullptr;

		return *this;
	}

	constexpr bool operator == (const LuaCPP& state) const
	{
		return GetHandle() == state.GetHandle();
	}
	constexpr bool operator != (const LuaCPP& state) const
	{
		return !operator==(state);
	}

private:
	template<typename T>
	static bool Pop(lua_State* lua, T& value)
	{
		static_assert(Get_Type<T>::Value != Types::None);

		if constexpr (!Is_Optional<T>::Value)
			if (lua_gettop(lua) == 0)
				return false;

		if constexpr (Is_Table<T>::Value)
			return Table_Pop(lua, value);
		else if constexpr (Is_Tuple<T>::Value)
			return Tuple_Pop(lua, value);
		else if constexpr (Is_Optional<T>::Value)
			return Optional_Pop(lua, value);
		else
		{
			if (!Peek<T>(lua, 1, value))
				return false;

			lua_pop(lua, 1);

			return true;
		}
	}
	static void Pop(lua_State* lua, size_t size = 1)
	{
		lua_pop(lua, static_cast<int>(size));
	}

	template<typename T>
	static bool Peek(lua_State* lua, size_t index, T& value)
	{
		if constexpr (!Is_Optional<T>::Value)
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
			value = static_cast<T>(lua_tonumber(lua, static_cast<int>(index)));

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
				if (auto string = lua_tostring(lua, static_cast<int>(index)))
				{
					value = string;

					return true;
				}
			}
			else if (size_t length; auto string = lua_tolstring(lua, static_cast<int>(index), &length))
			{
				if constexpr (Is_StringView<T>::Value)
					value = std::string_view(string, length);
				else
					value = std::string(string, length);

				return true;
			}
		}
		else if constexpr (Is_Table<T>::Value)
			return Table_Peek(lua, index, value);
		else if constexpr (Is_Tuple<T>::Value)
			return Tuple_Peek(lua, index, value);
		else if constexpr (Is_Function<T>::Value)
		{
			lua_pushvalue(lua, static_cast<int>(index));

			if (int reference = luaL_ref(lua, LUA_REGISTRYINDEX); reference != LUA_REFNIL)
			{
				value = T(lua, reference);

				return true;
			}

			lua_pop(lua, 1);
		}
		else if constexpr (Is_Optional<T>::Value)
			return Optional_Peek(lua, index, value);
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

	template<typename T>
	static int  Push(lua_State* lua, const T& value)
	{
		if constexpr (Is_Char<T>::Value)
		{
			lua_pushlstring(lua, &value, 1);

			return 1;
		}
		else if constexpr (Is_Number<T>::Value)
		{
			lua_pushnumber(lua, static_cast<lua_Number>(value));

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
				lua_pushstring(lua, value);
			else if constexpr (Is_StringView<T>::Value)
				lua_pushlstring(lua, value.data(), value.length());
			else
				lua_pushlstring(lua, value.c_str(), value.length());

			return 1;
		}
		else if constexpr (Is_Table<T>::Value)
		{
			Table_Push(lua, value);

			return 1;
		}
		else if constexpr (Is_Tuple<T>::Value)
			return Tuple_Push(lua, value);
		else if constexpr (Is_Function<T>::Value)
		{
			switch (value.context->type)
			{
				case FunctionTypes::C:
					lua_pushlightuserdata(lua, value.context.get());
					lua_pushcclosure(lua, &T::ExecuteC, 1);
					return 1;

				case FunctionTypes::Lua:
					if (auto value_type = lua_rawgeti(lua, LUA_REGISTRYINDEX, value.context->reference); value_type != LUA_TFUNCTION)
						throw Exception("LuaCPP::Push", "lua_rawgeti returned " + std::to_string(value_type));
					return 1;
			}
		}
		else if constexpr (Is_Optional<T>::Value)
			return Optional_Push(lua, value);
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

	/*
	static bool Table_Pop(lua_State* lua, Table& value)
	{
		// TODO: implement

		value.context->lua = lua;

		return false;
	}
	static bool Table_Peek(lua_State* lua, size_t index, Table& value)
	{
		// TODO: implement

		value.context->lua = lua;

		return false;
	}
	static int  Table_Push(lua_State* lua, const Table& value)
	{
		lua_createtable(lua, static_cast<int>(value.GetCount()), 0);

		auto stack_top = lua_gettop(lua);

		for (auto& field : value.context->fields)
		{
			assert(field.key.type != Types::None);
			assert(field.value.type != Types::None);

			auto stack_top_field = lua_gettop(lua);

			switch (field.key.type)
			{
				case Types::Null:          lua_pushnil(lua); break;
				case Types::Boolean:       lua_pushboolean(lua, field.key.boolean); break;
				case Types::LightUserData: lua_pushlightuserdata(lua, field.key.light_user_data); break;
				case Types::Number:        lua_pushnumber(lua, field.key.number); break;
				case Types::String:        lua_pushlstring(lua, field.key.string.c_str(), field.key.string.length()); break;
				case Types::Table:         Push<Table>(lua, *field.key.table); break;
				case Types::Function:      lua_pushcfunction(lua, field.key.function_c); break;
				case Types::UserData:      break; // TODO: implement
				case Types::Thread:        break; // TODO: implement
			}

			switch (field.value.type)
			{
				case Types::Null:          lua_pushnil(lua); break;
				case Types::Boolean:       lua_pushboolean(lua, field.value.boolean); break;
				case Types::LightUserData: lua_pushlightuserdata(lua, field.value.light_user_data); break;
				case Types::Number:        lua_pushnumber(lua, field.value.number); break;
				case Types::String:        lua_pushlstring(lua, field.value.string.c_str(), field.value.string.length()); break;
				case Types::Table:         Push<Table>(lua, *field.value.table); break;
				case Types::Function:      lua_pushcfunction(lua, field.value.function_c); break;
				case Types::UserData:      break; // TODO: implement
				case Types::Thread:        break; // TODO: implement
			}

			auto stack_top_field_size = lua_gettop(lua) - stack_top_field;

			if (stack_top_field_size == 1)
				lua_pop(lua, 1);
			else if (stack_top_field_size == 2)
				lua_settable(lua, stack_top);
		}

		return 1;
	}
	*/

	template<typename ... T>
	static constexpr bool Tuple_Pop(lua_State* lua, std::tuple<T ...>& value)
	{
		return Tuple_Pop(lua, value, std::make_index_sequence<sizeof...(T)> {});
	}
	template<size_t ... I, typename ... T>
	static constexpr void Tuple_Pop(lua_State* lua, std::tuple<T ...>& value, std::index_sequence<I ...>)
	{
		return (Pop<typename std::tuple_element<I, std::tuple<T ...>>::type>(lua, std::get<I>(value)) && ...);
	}
	template<typename ... T>
	static constexpr bool Tuple_Peek(lua_State* lua, size_t index, std::tuple<T ...>& value)
	{
		return Tuple_Peek(lua, index, value, std::make_index_sequence<sizeof...(T)> {});
	}
	template<typename ... T, size_t ... I>
	static constexpr bool Tuple_Peek(lua_State* lua, size_t index, std::tuple<T ...>& value, std::index_sequence<I ...>)
	{
		return (Peek<typename std::tuple_element<I, std::tuple<T ...>>::type>(lua, index + I, std::get<I>(value)) && ...);
	}
	template<typename ... T>
	static constexpr int  Tuple_Push(lua_State* lua, const std::tuple<T ...>& value)
	{
		return Tuple_Push<T ...>(lua, value, std::make_index_sequence<sizeof...(T)> {});
	}
	template<typename ... T, size_t ... I>
	static constexpr int  Tuple_Push(lua_State* lua, const std::tuple<T ...>& value, std::index_sequence<I ...>)
	{
		return (Push<T>(lua, std::get<I>(value)) + ...);
	}

	template<typename T>
	static constexpr bool Optional_Pop(lua_State* lua, Optional<T>& value)
	{
		value.is_set = Pop<T>(lua, value.value);

		return true;
	}
	template<typename T>
	static constexpr bool Optional_Peek(lua_State* lua, size_t index, Optional<T>& value)
	{
		value.is_set = Peek<T>(lua, index, value.value);

		return true;
	}
	template<typename T>
	static constexpr int  Optional_Push(lua_State* lua, const Optional<T>& value)
	{
		return value ? Push<T>(lua, *value) : 0;
	}

private:
	// @throw std::exception
	static bool FileExists(const std::string_view& path)
	{
		if (!std::filesystem::exists(path))
			return false;

		if (!std::filesystem::is_regular_file(path) && !std::filesystem::is_symlink(path))
			return false;

		return true;
	}
};
