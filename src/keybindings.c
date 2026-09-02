#include "keybindings.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <lauxlib.h>

#include "types.h"

static bool parse_modifier(const char *name,uint32_t *modifiers){
  if(!strcasecmp(name,"SHIFT"))
    *modifiers|=WLR_MODIFIER_SHIFT;
  else if(!strcasecmp(name,"CTRL")||!strcasecmp(name,"CONTROL"))
    *modifiers|=WLR_MODIFIER_CTRL;
  else if(!strcasecmp(name,"ALT"))
    *modifiers|=WLR_MODIFIER_ALT;
  else if(!strcasecmp(name,"SUPER")||!strcasecmp(name,"META")||
      !strcasecmp(name,"WIN"))
    *modifiers|=WLR_MODIFIER_LOGO;
  else if(!strcasecmp(name,"ALTGR"))
    *modifiers|=WLR_MODIFIER_MOD5;
  else
    return false;

  return true;
}

static bool parse_keybinding(const char *keys,uint32_t *modifiers,xkb_keysym_t *key){
  char *copy=strdup(keys);
  if(!copy)
    return false;

  char *saveptr=NULL;
  char *token=strtok_r(copy,"+",&saveptr);
  if(!token){
    free(copy);
    return false;
  }

  while(true){
    char *next=strtok_r(NULL,"+",&saveptr);
    if(next){
      if(!parse_modifier(token,modifiers)){
        free(copy);
        return false;
      }
      token=next;
      continue;
    }

    *key=xkb_keysym_from_name(token,XKB_KEYSYM_CASE_INSENSITIVE);
    free(copy);
    return *key!=XKB_KEY_NoSymbol;
  }
}

bool sandwl_keybinding_add(struct sandwl_server *server,lua_State *state,const char *keys,int callback_index){
  uint32_t modifiers=0;
  xkb_keysym_t key=XKB_KEY_NoSymbol;
  if(!parse_keybinding(keys,&modifiers,&key))
    return false;

  struct sandwl_keybinding *binding=calloc(1,sizeof(*binding));
  if(!binding)
    return false;

  binding->modifiers=modifiers;
  binding->key=key;
  binding->state=WL_KEYBOARD_KEY_STATE_PRESSED;

  lua_pushvalue(state,callback_index);
  binding->lua_callback_ref=luaL_ref(state,LUA_REGISTRYINDEX);
  wl_list_insert(&server->keybindings,&binding->link);
  return true;
}

void sandwl_keybindings_destroy(struct sandwl_server *server,lua_State *state){
  struct sandwl_keybinding *binding,*tmp;
  wl_list_for_each_safe(binding,tmp,&server->keybindings,link){
    if(state)
      luaL_unref(state,LUA_REGISTRYINDEX,binding->lua_callback_ref);
    wl_list_remove(&binding->link);
    free(binding);
  }
}
