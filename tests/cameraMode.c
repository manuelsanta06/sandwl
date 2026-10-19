#include <assert.h>
#include <stdio.h>
#include <linux/input-event-codes.h>
#include <lauxlib.h>
#include <wlr/interfaces/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>

#include "types.h"
#include "serverKeyboard.h"
#include "config/luaConfig.h"

static void run(lua_State *state,const char *code){
  if(luaL_dostring(state,code)!=LUA_OK){
    fprintf(stderr,"%s\n",lua_tostring(state,-1));
    assert(false);
  }
}

static lua_Integer counter(lua_State *state,const char *name){
  lua_getglobal(state,name);
  lua_Integer value=luaL_checkinteger(state,-1);
  lua_pop(state,1);
  return value;
}

static void key(struct wlr_keyboard *keyboard,uint32_t keycode,bool pressed){
  struct wlr_keyboard_key_event event={
    .keycode=keycode,
    .update_state=true,
    .state=pressed?WL_KEYBOARD_KEY_STATE_PRESSED:WL_KEYBOARD_KEY_STATE_RELEASED,
  };
  wlr_keyboard_notify_key(keyboard,&event);
}

int main(void){
  struct sandwl_server server={0};
  wl_list_init(&server.keyboards);
  wl_list_init(&server.pointers);
  wl_list_init(&server.keybindings);
  wl_list_init(&server.camera_mode.bindings);
  server.wl_display=wl_display_create();
  assert(server.wl_display);
  server.seat=wlr_seat_create(server.wl_display,"test");
  assert(server.seat);
  server.lua=sandwl_lua_create(&server);
  assert(server.lua);
  lua_State *state=sandwl_lua_get_state(server.lua);

  const struct wlr_keyboard_impl impl={.name="test"};
  struct wlr_keyboard keyboard;
  wlr_keyboard_init(&keyboard,&impl,"test");
  server_new_keyboard(&server,&keyboard.base);

  run(state,
    "normal=0; presses=0; releases=0; modifierReleases=0; errorsAfter=0\n"
    "sand.bind('W',function() normal=normal+1 end)\n"
    "sand.cameraMode.bind('W',sand.cameraMode.forward)\n"
    "sand.cameraMode.bind('A',sand.cameraMode.left)\n"
    "sand.cameraMode.bind('S',function(pressed)\n"
    "  sand.cameraMode.backward(pressed)\n"
    "  if pressed then presses=presses+1 else releases=releases+1 end\n"
    "end)\n"
    "sand.cameraMode.bind('CTRL+D',function(pressed)\n"
    "  sand.cameraMode.right(pressed)\n"
    "  if not pressed then modifierReleases=modifierReleases+1 end\n"
    "end)\n"
    "sand.bind('F1',sand.cameraMode.toggle)\n"
    "sand.cameraMode.bind('E',function() error('expected test error') end)\n"
    "sand.cameraMode.bind('R',function(pressed)\n"
    "  if pressed then sand.cameraMode.disable() else errorsAfter=errorsAfter+1 end\n"
    "end)\n"
    "assert(not pcall(sand.cameraMode.bind,'INVALID+W',function() end))\n"
    "assert(not pcall(sand.cameraMode.bind,'W','forward'))\n"
    "assert(not pcall(sand.cameraMode.forward,'true'))\n"
    "local camera=sand.cameras.current(); camera.jumpTo(0,0); camera.move(1,0)\n");

  key(&keyboard,KEY_W,true);
  assert(counter(state,"normal")==1);
  assert(!server.camera_mode.forward);
  run(state,"sand.cameraMode.enable()");
  key(&keyboard,KEY_W,false);
  assert(!server.camera_mode.forward);

  key(&keyboard,KEY_W,true);
  assert(server.camera_mode.forward);
  assert(counter(state,"normal")==1);
  key(&keyboard,KEY_A,true);
  assert(server.camera_mode.left);
  key(&keyboard,KEY_W,false);
  assert(!server.camera_mode.forward&&server.camera_mode.left);
  key(&keyboard,KEY_A,false);
  assert(!server.camera_mode.left);

  key(&keyboard,KEY_LEFTCTRL,true);
  key(&keyboard,KEY_D,true);
  assert(server.camera_mode.right);
  key(&keyboard,KEY_LEFTCTRL,false);
  key(&keyboard,KEY_D,false);
  assert(!server.camera_mode.right);
  assert(counter(state,"modifierReleases")==1);

  key(&keyboard,KEY_S,true);
  assert(server.camera_mode.backward);
  run(state,"sand.cameraMode.disable(); sand.cameraMode.disable()");
  assert(!server.camera_mode.enabled&&!server.camera_mode.backward);
  assert(counter(state,"presses")==1&&counter(state,"releases")==1);
  run(state,"sand.cameraMode.enable()");
  key(&keyboard,KEY_S,false);
  assert(counter(state,"releases")==1);

  key(&keyboard,KEY_F1,true);
  key(&keyboard,KEY_F1,false);
  assert(!server.camera_mode.enabled);
  key(&keyboard,KEY_W,true);
  key(&keyboard,KEY_W,false);
  assert(counter(state,"normal")==2);

  run(state,"sand.cameraMode.enable()");
  key(&keyboard,KEY_E,true);
  key(&keyboard,KEY_E,false);
  assert(lua_gettop(state)==0);
  key(&keyboard,KEY_R,true);
  assert(!server.camera_mode.enabled);
  assert(counter(state,"errorsAfter")==1);
  key(&keyboard,KEY_R,false);
  assert(counter(state,"errorsAfter")==1);

  run(state,"sand.cameraMode.enable()");
  key(&keyboard,KEY_S,true);
  wlr_keyboard_finish(&keyboard);
  assert(wl_list_empty(&server.keyboards));
  assert(!server.camera_mode.backward);
  assert(counter(state,"presses")==2&&counter(state,"releases")==2);

  wlr_keyboard_init(&keyboard,&impl,"test");
  server_new_keyboard(&server,&keyboard.base);
  key(&keyboard,KEY_W,true);
  assert(server.camera_mode.forward);
  sandwl_lua_destroy(server.lua);
  assert(!server.lua&&!server.camera_mode.forward);
  wlr_keyboard_finish(&keyboard);
  wl_display_destroy(server.wl_display);
  puts("Camera mode tests passed");
  return 0;
}
