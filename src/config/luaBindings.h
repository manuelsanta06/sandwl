#pragma once

#include <lua.h>

struct sandwl_server;

void sandwl_lua_register_api(lua_State *state,struct sandwl_server *server);
