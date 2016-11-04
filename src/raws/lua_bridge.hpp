#pragma once

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include <string>
#include <unordered_map>
#include <functional>

extern lua_State* lua_state;

void init_lua();
void exit_lua();
void load_lua_script(const std::string filename);

struct lua_lifecycle {
	lua_lifecycle() {
		init_lua();
	}

	~lua_lifecycle() {
		exit_lua();
	}
};

using lua_parser = std::unordered_map<std::string, const std::function<void()>>; 

void read_lua_table(const std::string &table, const std::function<void(std::string)> &on_start, const std::function<void(std::string)> &on_end, const lua_parser &parser);
void read_lua_table_inner(const std::string &table, const std::function<void(std::string)> &functor);
void read_lua_table_inner(const std::string &table, const std::function<void(std::string)> &on_start, const std::function<void(std::string)> &on_end, const lua_parser &parser);
inline std::string lua_str() { return lua_tostring(lua_state, -1); }
inline int lua_int() { return lua_tonumber(lua_state, -1); }
inline float lua_float() { return lua_tonumber(lua_state, -1); }

