#include "luaBindings.h"
#include "../keybindings.h"
#include "../types.h"

#include <errno.h>
#include <lua.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <lauxlib.h>

#include <wlr/util/log.h>

static int lua_sand_bind(lua_State *state){
  const char *keys=luaL_checkstring(state,1);
  luaL_checktype(state,2,LUA_TFUNCTION);
  struct sandwl_server *server=lua_touserdata(state,lua_upvalueindex(1));

  if(!server||!sandwl_keybinding_add(&server->keybindings,state,keys,2))
    return luaL_error(state,"unable to register keybinding '%s'",keys);

  return 0;
}

static int lua_sand_camera_mode_bind(lua_State *state){
  const char *keys=luaL_checkstring(state,1);
  luaL_checktype(state,2,LUA_TFUNCTION);
  struct sandwl_server *server=lua_touserdata(state,lua_upvalueindex(1));

  if(!sandwl_keybinding_add(&server->camera_mode.bindings,state,keys,2))
    return luaL_error(state,"unable to register camera binding '%s'",keys);

  return 0;
}

static int lua_sand_camera_mode_toggle(lua_State *state){
  struct sandwl_server *server=lua_touserdata(state,lua_upvalueindex(1));
  sandwl_camera_mode_set_enabled(server,state,!server->camera_mode.enabled);
  return 0;
}

static int lua_sand_camera_mode_enable(lua_State *state){
  struct sandwl_server *server=lua_touserdata(state,lua_upvalueindex(1));
  sandwl_camera_mode_set_enabled(server,state,true);
  return 0;
}

static int lua_sand_camera_mode_disable(lua_State *state){
  struct sandwl_server *server=lua_touserdata(state,lua_upvalueindex(1));
  sandwl_camera_mode_set_enabled(server,state,false);
  return 0;
}

static int lua_sand_camera_mode_forward(lua_State *state){
  luaL_checktype(state,1,LUA_TBOOLEAN);
  struct sandwl_server *server=lua_touserdata(state,lua_upvalueindex(1));
  server->camera_mode.forward=server->camera_mode.enabled&&lua_toboolean(state,1);
  return 0;
}

static int lua_sand_camera_mode_backward(lua_State *state){
  luaL_checktype(state,1,LUA_TBOOLEAN);
  struct sandwl_server *server=lua_touserdata(state,lua_upvalueindex(1));
  server->camera_mode.backward=server->camera_mode.enabled&&lua_toboolean(state,1);
  return 0;
}

static int lua_sand_camera_mode_left(lua_State *state){
  luaL_checktype(state,1,LUA_TBOOLEAN);
  struct sandwl_server *server=lua_touserdata(state,lua_upvalueindex(1));
  server->camera_mode.left=server->camera_mode.enabled&&lua_toboolean(state,1);
  return 0;
}

static int lua_sand_camera_mode_right(lua_State *state){
  luaL_checktype(state,1,LUA_TBOOLEAN);
  struct sandwl_server *server=lua_touserdata(state,lua_upvalueindex(1));
  server->camera_mode.right=server->camera_mode.enabled&&lua_toboolean(state,1);
  return 0;
}

static int lua_sand_on(lua_State *state){
  (void)state;
  return 0;
}

static int lua_sand_spawn(lua_State *state){
  const char *command=luaL_checkstring(state,1);
  pid_t pid=fork();

  if(pid<0){
    wlr_log(WLR_ERROR,"Unable to spawn command: %s",strerror(errno));
    lua_pushboolean(state,false);
    return 1;
  }if(pid==0){
    execl("/bin/sh","sh","-c",command,(char *)NULL);
    _exit(127);
  }

  lua_pushboolean(state,true);
  return 1;
}

static int lua_sand_log(lua_State *state){
  const char *message=luaL_checkstring(state,1);
  wlr_log(WLR_INFO,"[LUA] %s",message);
  return 0;
}

static int lua_sand_quit(lua_State *state){
  (void)state;
  exit(0);
}

static int lua_sand_version(lua_State *state){
  lua_pushstring(state,VERSION);
  return 1;
}

static int lua_sand_camera_jump_to(lua_State *state){
  (void)state;
  return 0;
}

static int lua_sand_camera_move(lua_State *state){
  (void)state;
  return 0;
}

struct sandwl_lua_camera{
  int placeholder;
};

static const luaL_Reg sand_functions[]={
  {"bind"     , lua_sand_bind},
  {"on"       , lua_sand_on},
  {"spawn"    , lua_sand_spawn},
  {"log"      , lua_sand_log},
  {"quit"     , lua_sand_quit},
  {"version"  , lua_sand_version},
  {NULL,NULL}
};

static const luaL_Reg camera_methods[]={
  {"jumpTo"   , lua_sand_camera_jump_to},
  {"move"     , lua_sand_camera_move},
  {NULL,NULL}
};

static const luaL_Reg camera_mode_functions[]={
  {"bind"     , lua_sand_camera_mode_bind},
  {"toggle"   , lua_sand_camera_mode_toggle},
  {"enable"   , lua_sand_camera_mode_enable},
  {"disable"  , lua_sand_camera_mode_disable},
  {"forward"  , lua_sand_camera_mode_forward},
  {"backward" , lua_sand_camera_mode_backward},
  {"left"     , lua_sand_camera_mode_left},
  {"right"    , lua_sand_camera_mode_right},
  {NULL,NULL}
};

static int lua_sand_cameras_current(lua_State *state){
  struct sandwl_lua_camera *camera=lua_newuserdatauv(state,sizeof(*camera),0);
  camera->placeholder=0;
  luaL_setmetatable(state,"sand.camera");
  return 1;
}

static const luaL_Reg cameras_functions[]={
  {"current",lua_sand_cameras_current},
  {NULL,NULL}
};

void sandwl_lua_register_api(lua_State *state,struct sandwl_server *server){
  luaL_newmetatable(state,"sand.camera");
  luaL_setfuncs(state,camera_methods,0);
  lua_pushvalue(state,-1);
  lua_setfield(state,-2,"__index");
  lua_pop(state,1);

  luaL_newlibtable(state,sand_functions);
  lua_pushlightuserdata(state,server);
  luaL_setfuncs(state,sand_functions,1);
  luaL_newlib(state,cameras_functions);
  lua_setfield(state,-2,"cameras");
  luaL_newlibtable(state,camera_mode_functions);
  lua_pushlightuserdata(state,server);
  luaL_setfuncs(state,camera_mode_functions,1);
  lua_setfield(state,-2,"cameraMode");
  lua_setglobal(state,"sand");
}
