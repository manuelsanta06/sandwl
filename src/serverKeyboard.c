#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>
#include <wlr/backend/session.h>

#include "types.h"
#include "serverKeyboard.h"


void keyboard_handle_modifiers(struct wl_listener *listener,void *data){
  (void)data;
  //raised on modifier pres
  struct sandwl_keyboard *keyboard=wl_container_of(listener,keyboard,modifiers);
  wlr_seat_set_keyboard(keyboard->server->seat,keyboard->wlr_keyboard);
  //pass modifier to client
  wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,&keyboard->wlr_keyboard->modifiers);
}

void keyboard_handle_key(struct wl_listener *listener,void *data){
  //raised on key pres
  struct sandwl_keyboard *keyboard=wl_container_of(listener,keyboard,key);
  struct sandwl_server *server=keyboard->server;
  struct wlr_keyboard_key_event *event=data;
  struct wlr_seat *seat=server->seat;

  //Translate from libinput keycode to xkbcommon
  uint32_t keycode=event->keycode+8;
  //Get a list of keysyms based on the keymap for this keyboard
  const xkb_keysym_t *syms;
  int nsyms=xkb_state_key_get_syms(keyboard->wlr_keyboard->xkb_state,keycode,&syms);

  bool handled=false;
  uint32_t modifiers=wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);
  //tty keybindings check
  if(!handled&&server->session){
    for(int i=0;i<nsyms;i++){
      if(syms[i]>=XKB_KEY_XF86Switch_VT_1&&syms[i]<=XKB_KEY_XF86Switch_VT_12){
        wlr_session_change_vt(server->session,syms[i]-XKB_KEY_XF86Switch_VT_1+1);
        handled=true;
        break;
      }
    }
  }
  if(false){
    //handle system keybindings here, before giving them to toplevels
    //set 'handled' to true if keys get used
  }
  //examble
  if((modifiers&WLR_MODIFIER_ALT)&&event->state==WL_KEYBOARD_KEY_STATE_PRESSED){
    for (int i=0;i<nsyms;i++){
      switch(syms[i]){
        case XKB_KEY_q:
          if(fork()==0)
            execl("/bin/sh","/bin/sh","-c","kitty",(void*)NULL);
          break;
        default:
          break;
      }
    }
  }
  if(!handled){
    //pass keys to client
    wlr_seat_set_keyboard(seat,keyboard->wlr_keyboard);
    wlr_seat_keyboard_notify_key(seat,event->time_msec,event->keycode,event->state);
  }
}
void keyboard_handle_destroy(struct wl_listener *listener,void *data){
  (void)data;
  struct sandwl_keyboard *keyboard=wl_container_of(listener,keyboard,destroy);
  wl_list_remove(&keyboard->modifiers.link);
  wl_list_remove(&keyboard->key.link);
  wl_list_remove(&keyboard->destroy.link);
  wl_list_remove(&keyboard->link);
  free(keyboard);
}

void server_new_keyboard(struct sandwl_server *server,struct wlr_input_device *device){
  struct wlr_keyboard *wlr_keyboard=wlr_keyboard_from_input_device(device);
  struct sandwl_keyboard *keyboard=calloc(1,sizeof(*keyboard));
  keyboard->server=server;
  keyboard->wlr_keyboard=wlr_keyboard;

  //prepare an XKB keymap and assign it to the keyboard. assumes default layout
  struct xkb_context *context=xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  struct xkb_keymap *keymap=xkb_keymap_new_from_names(context,NULL,XKB_KEYMAP_COMPILE_NO_FLAGS);

  wlr_keyboard_set_keymap(wlr_keyboard,keymap);
  xkb_keymap_unref(keymap);
  xkb_context_unref(context);
  wlr_keyboard_set_repeat_info(wlr_keyboard,25,600);

  //listeners for keyboard events
  keyboard->modifiers.notify=keyboard_handle_modifiers;
  wl_signal_add(&wlr_keyboard->events.modifiers,&keyboard->modifiers);
  keyboard->key.notify=keyboard_handle_key;
  wl_signal_add(&wlr_keyboard->events.key,&keyboard->key);
  keyboard->destroy.notify=keyboard_handle_destroy;
  wl_signal_add(&device->events.destroy, &keyboard->destroy);

  wlr_seat_set_keyboard(server->seat,keyboard->wlr_keyboard);

  //add the keyboard to list of keyboards
  wl_list_insert(&server->keyboards,&keyboard->link);
}
