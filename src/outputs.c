#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <wayland-util.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>

#include "types.h"
#include "outputs.h"

void output_destroy(struct wl_listener *listener,void *data){
  (void)data;
  struct sandwl_output *output=wl_container_of(listener,output,destroy);

  wl_list_remove(&output->frame.link);
  wl_list_remove(&output->request_state.link);
  wl_list_remove(&output->destroy.link);
  wl_list_remove(&output->link);
  free(output);
}

void output_request_state(struct wl_listener *listener,void *data){
  // This function is called when the backend requests a new state for
  // the output. For example, Wayland and X11 backends request a new mode
  // when the output window is resized.
  struct sandwl_output *output=wl_container_of(listener,output,request_state);
  const struct wlr_output_event_request_state *event=data;
  wlr_output_commit_state(output->wlr_output,event->state);
}

void output_frame(struct wl_listener *listener,void *data){
  (void)data;
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

void server_new_output(struct wl_listener *listener,void *data){
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

  wlr_output->data=output;

  for(int i=0;i<4;i++)wl_list_init(&output->layers[i]);

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

void arrange_layers(struct sandwl_output *output){
  struct wlr_box full={0};
  wlr_output_layout_get_box(output->server->output_layout,output->wlr_output,&full);
  
  struct wlr_box usable=full;

  for(int i=0;i<4;i++){
    struct sandwl_layer_surface *layer;
    wl_list_for_each(layer,&output->layers[i],link){
      wlr_scene_layer_surface_v1_configure(layer->scene_layer_surface,&full,&usable);
    }
  }

  output->usable_area=usable;
}
