#pragma once

#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include "types.h"
#include "utilies.h"
#include "processCursor.h"


static void server_cursor_motion(struct wl_listener *listener,void *data){
  //forwarded by the cursor when a pointer emits _relative_ pointer motion event(ej a delta)
  struct sandwl_server *server=wl_container_of(listener,server,cursor_motion);
  struct wlr_pointer_motion_event *event=data;
  wlr_cursor_move(server->cursor,&event->pointer->base,event->delta_x,event->delta_y);
  process_cursor_motion(server,event->time_msec);
}

static void server_cursor_motion_absolute(struct wl_listener *listener,void *data){
  //this event is forwarded by the cursor when a pointer emits an _absolute_ motion event
  //from 0..1 on each axis
  //this happens when wlroots is running under a Wayland window rather than KMS+DRM
  //you could enter the window from any edge, so we have to warp the mouse there
  //there is also some hardware which
  //emits these events
  struct sandwl_server *server=wl_container_of(listener,server,cursor_motion_absolute);
  struct wlr_pointer_motion_absolute_event *event=data;
  wlr_cursor_warp_absolute(server->cursor,&event->pointer->base,event->x,event->y);
  process_cursor_motion(server,event->time_msec);
}

static void server_cursor_button(struct wl_listener *listener,void *data){
  //forwarded by the cursor when a pointer emits a button event
  struct sandwl_server *server=wl_container_of(listener,server,cursor_button);
  struct wlr_pointer_button_event *event=data;
  //notify the client with pointer focus
  wlr_seat_pointer_notify_button(server->seat,event->time_msec,event->button,event->state);
  if(event->state==WL_POINTER_BUTTON_STATE_RELEASED){
    //if button released exit interactive move/resize mode
    reset_cursor_mode(server);
  }else{
    //focus that client if the button was _pressed_
    double sx,sy;
    struct wlr_surface *surface=NULL;
    struct sandwl_toplevel *toplevel=desktop_toplevel_at(server,
      server->cursor->x,server->cursor->y,&surface,&sx,&sy);
    focus_toplevel(toplevel);
  }
}

static void server_cursor_axis(struct wl_listener *listener,void *data){
  //forwarded by the cursor when a pointer emits an axis event
  //for example when you move the scroll wheel
  struct sandwl_server *server=wl_container_of(listener,server,cursor_axis);
  struct wlr_pointer_axis_event *event=data;
  //notify the client with pointer focus of the axis event
  wlr_seat_pointer_notify_axis(server->seat,
      event->time_msec,event->orientation,event->delta,
      event->delta_discrete,event->source,event->relative_direction);
}

static void server_cursor_frame(struct wl_listener *listener,void *data){
  //forwarded by the cursor whe a pointer emits an frame event
  struct sandwl_server *server=wl_container_of(listener,server,cursor_frame);
  //notify the client with pointer focus of the frame event
  wlr_seat_pointer_notify_frame(server->seat);
}

static void server_new_pointer(struct sandwl_server *server,struct wlr_input_device *device){
  //pointer configuration should be applied here
  wlr_cursor_attach_input_device(server->cursor,device);
}
