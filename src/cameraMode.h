#pragma once

#include "keybindings.h"

struct sandwl_keyboard;

struct sandwl_camera_mode{
  struct wl_list bindings;
  bool enabled;
  bool forward;
  bool backward;
  bool left;
  bool right;
};

void sandwl_camera_mode_set_enabled(struct sandwl_server *server,lua_State *state,bool enabled);

bool sandwl_camera_mode_handle_key(struct sandwl_keyboard *keyboard,lua_State *state,
  const xkb_keysym_t *syms,size_t nsyms,uint32_t modifiers,uint32_t keycode,
  enum wl_keyboard_key_state key_state);

void sandwl_camera_mode_release_keyboard(struct sandwl_keyboard *keyboard,lua_State *state);
void sandwl_camera_mode_destroy(struct sandwl_server *server,lua_State *state);
