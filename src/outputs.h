#pragma once

#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
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

static void output_destroy(struct wl_listener *listener,void *data){
  struct sandwl_output *output=wl_container_of(listener,output,destroy);

  wl_list_remove(&output->frame.link);
  wl_list_remove(&output->request_state.link);
  wl_list_remove(&output->destroy.link);
  wl_list_remove(&output->link);
  free(output);
}

static void output_request_state(struct wl_listener *listener,void *data){
  // This function is called when the backend requests a new state for
  // the output. For example, Wayland and X11 backends request a new mode
  // when the output window is resized.
  struct sandwl_output *output=wl_container_of(listener,output,request_state);
  const struct wlr_output_event_request_state *event=data;
  wlr_output_commit_state(output->wlr_output,event->state);
}

static void output_frame(struct wl_listener *listener,void *data){
  //runs each time an output wants to render a frame
  struct sandwl_output *output=wl_container_of(listener,output,frame);
  struct wlr_scene *scene=output->server->scene;

  struct wlr_scene_output *scene_output=wlr_scene_get_scene_output(
    scene,output->wlr_output);

  //Render the scene if needed and commit the output
  wlr_scene_output_commit(scene_output,NULL);

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC,&now);
  wlr_scene_output_send_frame_done(scene_output,&now);
}

static void server_new_output(struct wl_listener *listener,void *data){
  struct sandwl_server *server=wl_container_of(listener,server,new_output);
  struct wlr_output *wlr_output=data;

  wlr_output_init_render(wlr_output,server->allocator,server->renderer);

  struct wlr_output_state state;
  wlr_output_state_init(&state);
  wlr_output_state_set_enabled(&state,true);

  //set output mode(size and refresh rate)
  struct wlr_output_mode *mode=wlr_output_preferred_mode(wlr_output);
  if(mode!=NULL){
    wlr_output_state_set_mode(&state,mode);
  }

  wlr_output_commit_state(wlr_output, &state);
  wlr_output_state_finish(&state);

  struct sandwl_output *output=calloc(1,sizeof(*output));
  output->wlr_output=wlr_output;
  output->server=server;

  output->frame.notify=output_frame;
  wl_signal_add(&wlr_output->events.frame,&output->frame);
  output->request_state.notify=output_request_state;
  wl_signal_add(&wlr_output->events.request_state,&output->request_state);
  output->destroy.notify=output_destroy;
  wl_signal_add(&wlr_output->events.destroy,&output->destroy);

  wl_list_insert(&server->outputs,&output->link);

  //adds the output to the layout
  //'wlr_output_layout_add_auto' arranges outputs from left to right
  struct wlr_output_layout_output *l_output=wlr_output_layout_add_auto(
    server->output_layout,wlr_output);
  struct wlr_scene_output *scene_output=wlr_scene_output_create(server->scene,wlr_output);
  wlr_scene_output_layout_add_output(server->scene_layout,l_output,scene_output);
}
