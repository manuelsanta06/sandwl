#include "luaConfig.h"
#include "luaBindings.h"
#include "keybindings.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <wlr/util/log.h>

#include "types.h"

struct sandwl_lua {
  lua_State *state;
  struct sandwl_server *server;
};

static char *default_config_path(void){
  const char *home=getenv("HOME");
  const char suffix[]="/.config/sandwl/config.lua";

  if(!home||home[0]=='\0')
    return NULL;

  size_t path_length=strlen(home)+sizeof(suffix);
  char *path=malloc(path_length);
  if(!path)
    return NULL;

  snprintf(path,path_length,"%s%s",home,suffix);
  return path;
}

static enum sandwl_lua_config_result check_config_file(const char *path){
  struct stat file_stat;
  if(stat(path,&file_stat)==-1){
    if(errno==ENOENT){
      wlr_log(WLR_INFO,"Lua config not found: %s",path);
      return SANDWL_LUA_CONFIG_NOT_FOUND;
    }

    wlr_log(WLR_ERROR,"Unable to inspect Lua config %s: %s",
      path,strerror(errno));
    return SANDWL_LUA_CONFIG_FAILED;
  }

  if(!S_ISREG(file_stat.st_mode)){
    wlr_log(WLR_ERROR,"Lua config is not a regular file: %s",path);
    return SANDWL_LUA_CONFIG_FAILED;
  }

  return SANDWL_LUA_CONFIG_LOADED;
}

struct sandwl_lua *sandwl_lua_create(struct sandwl_server *server){
  struct sandwl_lua *lua=calloc(1,sizeof(*lua));
  if(!lua){
    wlr_log(WLR_ERROR,"Unable to allocate Lua runtime");
    return NULL;
  }

  lua->state=luaL_newstate();
  if(!lua->state){
    wlr_log(WLR_ERROR,"Unable to create Lua state");
    free(lua);
    return NULL;
  }

  lua->server=server;
  luaL_openlibs(lua->state);
  sandwl_lua_register_api(lua->state,server);
  return lua;
}

lua_State *sandwl_lua_get_state(struct sandwl_lua *lua){
  return lua?lua->state:NULL;
}

enum sandwl_lua_config_result sandwl_lua_load_config(struct sandwl_lua *lua, const char *path){
  if(!lua||!lua->state)
    return SANDWL_LUA_CONFIG_FAILED;

  if(!path){
    path=default_config_path();
    if(!path){
      wlr_log(WLR_ERROR,"Unable to determine Lua config path: HOME is unset");
      return SANDWL_LUA_CONFIG_FAILED;
    }
  }

  enum sandwl_lua_config_result file_result=check_config_file(path);
  if(file_result!=SANDWL_LUA_CONFIG_LOADED){
    return file_result;
  }

  if(luaL_loadfile(lua->state,path)!=LUA_OK){
    const char *error=lua_tostring(lua->state,-1);
    wlr_log(WLR_ERROR,"Unable to load Lua config %s: %s",
      path,error ? error : "unknown Lua error");
    lua_settop(lua->state,0);
    return SANDWL_LUA_CONFIG_FAILED;
  }

  // Config files are arbitrary Lua code, so protect execution with pcall
  // this turns syntax/runtime errors into normal startup errors instead of allowing an uncaught 
  // Lua error to abort the compositor
  if(lua_pcall(lua->state,0,LUA_MULTRET,0)!=LUA_OK){
    const char *error=lua_tostring(lua->state,-1);
    wlr_log(WLR_ERROR,"Error while executing Lua config %s: %s",path,error?error:"unknown Lua error");
    lua_settop(lua->state,0);
    return SANDWL_LUA_CONFIG_FAILED;
  }

  lua_settop(lua->state,0);
  wlr_log(WLR_INFO,"Loaded Lua config: %s",path);
  return SANDWL_LUA_CONFIG_LOADED;
}

void sandwl_lua_destroy(struct sandwl_lua *lua){
  if(!lua)
    return;
  sandwl_keybindings_destroy(lua->server,lua->state);
  if(lua->state)
    lua_close(lua->state);
  free(lua);
}
