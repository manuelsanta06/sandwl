#pragma once

#include <lua.h>
#include <stdbool.h>
#include <stddef.h>
#include <wayland-server-protocol.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_keyboard.h>
#include <xkbcommon/xkbcommon.h>

struct sandwl_server;

struct sandwl_keybinding{
  struct wl_list link;
  uint32_t modifiers;
  xkb_keysym_t key;
  enum wl_keyboard_key_state state;
  int lua_callback_ref;
};

bool sandwl_keybinding_add(struct sandwl_server *server,lua_State *state,
  const char *keys,int callback_index);

bool sandwl_keybindings_handle(struct sandwl_server *server,lua_State *state,
  const xkb_keysym_t *syms,size_t nsyms,uint32_t modifiers,
  enum wl_keyboard_key_state key_state);

void sandwl_keybindings_destroy(struct sandwl_server *server,lua_State *state);
