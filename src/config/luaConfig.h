#pragma once

#include <stdbool.h>

struct sandwl_lua;

enum sandwl_lua_config_result {
  SANDWL_LUA_CONFIG_LOADED,
  SANDWL_LUA_CONFIG_NOT_FOUND,
  SANDWL_LUA_CONFIG_FAILED,
};

/* Create an empty Lua runtime */
struct sandwl_lua *sandwl_lua_create(void);

/*
 * Execute a configuration file. Passing NULL uses the default path:
 * ~/.config/sandwl/config.lua
 */
enum sandwl_lua_config_result sandwl_lua_load_config(struct sandwl_lua *lua,const char *path);

void sandwl_lua_destroy(struct sandwl_lua *lua);
