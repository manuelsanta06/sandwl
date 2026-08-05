#include <stdlib.h>
#include <wayland-util.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>

#include "types.h"
#include "layerShell.h"
#include "outputs.h"

#include <wlr/util/log.h>
static void layer_surface_map(struct wl_listener *listener,void *data){
  struct sandwl_layer_surface *layer_surface=wl_container_of(listener,layer_surface,map);
  struct sandwl_server *server=layer_surface->server;
  if(layer_surface->layer_surface->current.keyboard_interactive){
    struct wlr_keyboard *keyboard=wlr_seat_get_keyboard(server->seat);
    if(keyboard){
      wlr_seat_keyboard_notify_enter(server->seat,layer_surface->layer_surface->surface,
        keyboard->keycodes,keyboard->num_keycodes,&keyboard->modifiers);
    }
  }
  struct wlr_output *output=layer_surface->layer_surface->output;
  if(!output){
    struct sandwl_output *firstOutput=wl_container_of(&server->outputs.next,firstOutput,link);
    output=firstOutput->wlr_output;
    layer_surface->layer_surface->output=output;
  }
  struct sandwl_output *sandwl_out=output->data;
  wl_list_insert(&sandwl_out->layers[layer_surface->layer_surface->current.layer],&layer_surface->link);

  wlr_scene_node_set_enabled(&layer_surface->scene_tree->node,true);
  //TODO:Exclusive Zones
}

static void layer_surface_unmap(struct wl_listener *listener,void *data){
  struct sandwl_layer_surface *layer_surface=wl_container_of(listener,layer_surface,unmap);
  wl_list_remove(&layer_surface->link);
  wlr_scene_node_set_enabled(&layer_surface->scene_tree->node,false);
}

static void layer_surface_destroy(struct wl_listener *listener,void *data){
  struct sandwl_layer_surface *layer_surface=wl_container_of(listener,layer_surface,destroy);

  wl_list_remove(&layer_surface->map.link);
  wl_list_remove(&layer_surface->unmap.link);
  wl_list_remove(&layer_surface->destroy.link);
  wl_list_remove(&layer_surface->surface_commit.link);

  // Liberamos nuestra estructura
  free(layer_surface);
}

static void layer_surface_commit(struct wl_listener *listener,void *data){
  struct sandwl_layer_surface *layer_surface=wl_container_of(listener,layer_surface,surface_commit);
  struct wlr_layer_surface_v1 *wlr_layer_surface=layer_surface->layer_surface;

  if(wlr_layer_surface->initial_commit){
    if(!wlr_layer_surface->output){
      if(wl_list_empty(&layer_surface->server->outputs))return;
      struct sandwl_output *firstOutput=wl_container_of(
        layer_surface->server->outputs.next,firstOutput,link);
      wlr_layer_surface->output=firstOutput->wlr_output;
    }
    uint32_t width=wlr_layer_surface->pending.desired_width;
    uint32_t height=wlr_layer_surface->pending.desired_height;

    if(width ==0)width=wlr_layer_surface->output->width;
    if(height==0)height=wlr_layer_surface->output->height;

    wlr_layer_surface_v1_configure(wlr_layer_surface,width,height);
  }
  if(wlr_layer_surface->output){
    arrange_layers(wlr_layer_surface->output->data);
  }
}

void server_new_layer_surface(struct wl_listener *listener,void *data){
  struct sandwl_server *server=wl_container_of(listener,server,new_layer_surface);
  struct wlr_layer_surface_v1 *wlr_layer_surface=data;

  struct sandwl_layer_surface *layer_surface=calloc(1,sizeof(*layer_surface));
  layer_surface->server=server;
  layer_surface->layer_surface=wlr_layer_surface;

  struct wlr_scene_tree *parent_tree;
  switch(wlr_layer_surface->pending.layer){
    case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
      parent_tree=server->scene_background;
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
      parent_tree=server->scene_bottom;
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_TOP:
      parent_tree=server->scene_top;
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
      parent_tree=server->scene_overlay;
      break;
  }

  layer_surface->scene_layer_surface=wlr_scene_layer_surface_v1_create(parent_tree,wlr_layer_surface);
  layer_surface->scene_tree=layer_surface->scene_layer_surface->tree;
  
  layer_surface->scene_tree->node.data=layer_surface;
  wlr_layer_surface->data=layer_surface->scene_tree;

  layer_surface->map.notify=layer_surface_map;
  wl_signal_add(&wlr_layer_surface->surface->events.map,&layer_surface->map);
  layer_surface->unmap.notify=layer_surface_unmap;
  wl_signal_add(&wlr_layer_surface->surface->events.unmap,&layer_surface->unmap);
  layer_surface->destroy.notify=layer_surface_destroy;
  wl_signal_add(&wlr_layer_surface->events.destroy,&layer_surface->destroy);
  layer_surface->surface_commit.notify=layer_surface_commit;
  wl_signal_add(&wlr_layer_surface->surface->events.commit,&layer_surface->surface_commit);
}
