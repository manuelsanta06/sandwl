#include "cameraMode.h"
#include "types.h"

#include <stdlib.h>
#include <math.h>
#include <lauxlib.h>
#include <wlr/util/log.h>

struct sandwl_camera_key{
  struct wl_list link;
  struct sandwl_keybinding *binding;
  uint32_t keycode;
  bool active;
};

static void call_binding(lua_State *state,struct sandwl_keybinding *binding,bool pressed){
  if(!state)
    return;

  lua_rawgeti(state,LUA_REGISTRYINDEX,binding->lua_callback_ref);
  lua_pushboolean(state,pressed);
  if(lua_pcall(state,1,0,0)!=LUA_OK){
    const char *error=lua_tostring(state,-1);
    wlr_log(WLR_ERROR,"Error while executing camera binding: %s",
      error?error:"unknown Lua error");
    lua_pop(state,1);
  }
}

static void clear_directions(struct sandwl_camera_mode *mode){
  mode->forward=false;
  mode->backward=false;
  mode->left=false;
  mode->right=false;
}

void camera_mode_add_motion(struct sandwl_server *server,double dx,double dy){
  if(!server->camera_mode.enabled||!isfinite(dx)||!isfinite(dy))
    return;

  const double sensitivity=0.1;
  server->camera_mode.yaw_delta+=dx*sensitivity;
  server->camera_mode.pitch_delta-=dy*sensitivity;
}

void sandwl_camera_mode_clear_motion(struct sandwl_server *server){
  server->camera_mode.yaw_delta=0;
  server->camera_mode.pitch_delta=0;

  struct sandwl_pointer *pointer;
  wl_list_for_each(pointer,&server->pointers,link){
    pointer->pending_absolute_dx=0;
    pointer->pending_absolute_dy=0;
  }
}

static void reset_motion(struct sandwl_server *server){
  sandwl_camera_mode_clear_motion(server);
  struct sandwl_pointer *pointer;
  wl_list_for_each(pointer,&server->pointers,link)
    pointer->has_absolute_position=false;
}

void sandwl_camera_mode_set_enabled(struct sandwl_server *server,lua_State *state,bool enabled){
  struct sandwl_camera_mode *mode=&server->camera_mode;
  if(mode->enabled==enabled)
    return;

  mode->enabled=enabled;
  clear_directions(mode);
  reset_motion(server);
  if(enabled)
    return;

  struct sandwl_keyboard *keyboard;
  wl_list_for_each(keyboard,&server->keyboards,link){
    struct sandwl_camera_key *key;
    wl_list_for_each(key,&keyboard->camera_keys,link){
      if(!key->active)
        continue;

      // Keep the key until its physical release, but finish the Lua action now.
      key->active=false;
      call_binding(state,key->binding,false);
    }
  }
}

bool sandwl_camera_mode_handle_key(struct sandwl_keyboard *keyboard,lua_State *state,
    const xkb_keysym_t *syms,size_t nsyms,uint32_t modifiers,uint32_t keycode,
    enum wl_keyboard_key_state key_state){
  struct sandwl_camera_key *key;
  wl_list_for_each(key,&keyboard->camera_keys,link){
    if(key->keycode!=keycode)
      continue;

    if(key_state==WL_KEYBOARD_KEY_STATE_RELEASED){
      wl_list_remove(&key->link);
      if(key->active)
        call_binding(state,key->binding,false);
      free(key);
    }
    return true;
  }

  struct sandwl_camera_mode *mode=&keyboard->server->camera_mode;
  if(!mode->enabled||!state||key_state!=WL_KEYBOARD_KEY_STATE_PRESSED)
    return false;

  struct sandwl_keybinding *binding;
  wl_list_for_each(binding,&mode->bindings,link){
    if(!sandwl_keybinding_matches(binding,syms,nsyms,modifiers))
      continue;

    key=calloc(1,sizeof(*key));
    if(!key){
      wlr_log(WLR_ERROR,"Unable to track camera key press");
      return false;
    }

    key->binding=binding;
    key->keycode=keycode;
    key->active=true;
    wl_list_insert(&keyboard->camera_keys,&key->link);
    call_binding(state,binding,true);
    return true;
  }
  return false;
}

void sandwl_camera_mode_release_keyboard(struct sandwl_keyboard *keyboard,lua_State *state){
  struct sandwl_camera_key *key,*tmp;
  wl_list_for_each_safe(key,tmp,&keyboard->camera_keys,link){
    wl_list_remove(&key->link);
    if(key->active)
      call_binding(state,key->binding,false);
    free(key);
  }
}

void sandwl_camera_mode_destroy(struct sandwl_server *server,lua_State *state){
  server->camera_mode.enabled=false;
  clear_directions(&server->camera_mode);
  reset_motion(server);

  struct sandwl_keyboard *keyboard;
  wl_list_for_each(keyboard,&server->keyboards,link)
    sandwl_camera_mode_release_keyboard(keyboard,NULL);

  sandwl_keybindings_destroy(&server->camera_mode.bindings,state);
}
