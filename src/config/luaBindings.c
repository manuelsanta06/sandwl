#include "luaBindings.h"

#include <errno.h>
#include <lua.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <lauxlib.h>

#include <wlr/util/log.h>

static int lua_sand_bind(lua_State *state){
  (void)state;
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
  return 0;
}

static int lua_sand_version(lua_State *state){
  (void)state;
  return 0;
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

void sandwl_lua_register_api(lua_State *state){
  luaL_newmetatable(state,"sand.camera");
  luaL_setfuncs(state,camera_methods,0);
  lua_pushvalue(state,-1);
  lua_setfield(state,-2,"__index");
  lua_pop(state,1);

  luaL_newlib(state,sand_functions);
  luaL_newlib(state,cameras_functions);
  lua_setfield(state,-2,"cameras");
  lua_setglobal(state,"sand");
}
