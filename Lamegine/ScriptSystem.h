#pragma once
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <string>
#include <iostream>

#pragma comment(lib, "ScriptingAPI")

class ScriptingBase {
protected:
	lua_State* L;
public:
	void push(bool state) {
		lua_pushboolean(L, state);
	}
	void push(int number) {
		lua_pushinteger(L, number);
	}
	void push(float number) {
		lua_pushnumber(L, (lua_Number)number);
	}
	void push(const char *text) {
		lua_pushstring(L, text);
	}
	void push(const std::string&& text) {
		lua_pushstring(L, text.c_str());
	}
};

class FunctionHandler : public ScriptingBase {
private:
	lua_State *L;
	int counter;
public:
	FunctionHandler(lua_State *state) : L(state) {}
	void* getLuaState() { return L; }
	bool getArg(int argNo, int& arg) {
		argNo++;
		int count = lua_gettop(L);
		if (argNo > count) return false;
		arg = (int)lua_tointeger(L, argNo);
		return true;
	}
	bool getArg(int argNo, double& arg) {
		argNo++;
		int count = lua_gettop(L);
		if (argNo > count) return false;
		arg = lua_tonumber(L, argNo);
		return true;
	}
	bool getArg(int argNo, void*& arg) {
		argNo++;
		int count = lua_gettop(L);
		if (argNo > count) return false;
		arg = lua_touserdata(L, argNo);
		return true;
	}
	bool getArg(int argNo, const char*& arg) {
		argNo++;
		int count = lua_gettop(L);
		if (argNo > count) return false;
		arg = lua_tostring(L, argNo);
		return true;
	}
	bool getArg(int argNo, IObject3D*& obj) {
		argNo++;
		int count = lua_gettop(L);
		if (argNo > count) return false;
		obj = (IObject3D*)lua_touserdata(L, argNo);
		return true;
	}
	template<class First, class... Param> bool getArgs(First& first, Param&... p) {
		counter = 0;
		return sub_getArgs(first, p...);
	}
	template<class First> bool getArgs(First& f) {
		return getArg(0, f);
	}
	template<class First, class... Param> int ret(const First f, const Param ...p) {
		return ret(f) + ret(p...);
	}
	template<class First> int sub_ret(const First f) {
		push(f);
		return 1;
	}
private:
	template<class First, class... Param> bool sub_getArgs(First& first, Param& ...p) {
		return sub_getArgs(first) && sub_getArgs(p...);
	}
	template<class First> bool sub_getArgs(First& f) {
		return getArg(counter++, f);
	}
	template<class First, class... Param> int sub_ret(const First f, const Param ...p) {
		sub_ret(f);
		return 1 + sub_ret(p...);
	}
	bool getArgs() {
		return true;
	}
};

class ScriptSystem : public ScriptingBase {

	struct Any {
		union {
			const char *str;
			int intVal;
			lua_Number numVal;
			bool boolVal;
		} value;
		unsigned char type;
	};

private:
	int argCounter;
public:
	ScriptSystem();
	~ScriptSystem();

	bool loadFile(const char *name);

	void registerFunction(const char *name, int(*function)(lua_State*));

	template<class T> void setGlobal(const char *name, T val) {
		push(val);
		lua_setglobal(L, name);
	}

	template<class T> void pushArg(T arg) {
		argCounter++;
		push(arg);
	}

	bool endCall() {
		if (lua_pcall(L, argCounter, 1, 0)==0) {
			lua_pop(L, 1);
			return true;
		}
		std::cout << " [ScriptSystem] function call failed" << std::endl;
		return false;
	}

	bool beginCall(const char *name) {
		argCounter = 0;
		if (!lua_getglobal(L, name)) {
			std::cout << " [ScriptSystem] calling undefined function: " << name << std::endl;
			return false;
		}
		return true;
	}

	bool call(const char *name) {
		if (beginCall(name)) {
			endCall();
			return true;
		}
		return false;
	}

	template<class ...Param>
	bool call(const char *name, const Param ...p1) {
		if (!beginCall(name)) return false;
		subcall(p1...);
		return true;
	}

	void subcall() {
		endCall();
	}

	template<class First, class ...Param>
	void subcall(const First f, const Param ...p) {
		pushArg(f);
		subcall(p...);
	}
};