#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_scene.h>

#include <stdlib.h>

#include "types.h"
#include "xdgToplevel.h"


void decoration_handle_destroy(struct wl_listener *listener,void *data){
  struct sandwl_decoration *deco=wl_container_of(listener,deco,destroy);
  if(deco->toplevel)
    deco->toplevel->decoration=NULL;

  wl_list_remove(&deco->request_mode.link);
  wl_list_remove(&deco->destroy.link);
  free(deco);
}

void decoration_handle_request_mode(struct wl_listener *listener,void *data){
  struct sandwl_decoration *deco=wl_container_of(listener,deco,request_mode);

  if(deco->wlr_decoration->toplevel->base->initialized){
    wlr_xdg_toplevel_decoration_v1_set_mode(deco->wlr_decoration,WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
  }
}

void server_new_xdg_decoration(struct wl_listener *listener,void *data){
  struct wlr_xdg_toplevel_decoration_v1 *wlr_deco=data;
  struct wlr_xdg_surface *xdg_surface=wlr_deco->toplevel->base;
  struct wlr_scene_tree *scene_tree=xdg_surface->data;
  struct sandwl_toplevel *toplevel=scene_tree->node.data;

  struct sandwl_decoration *deco=calloc(1,sizeof(*deco));
  deco->wlr_decoration=wlr_deco;
  deco->toplevel=toplevel;
  toplevel->decoration=deco;

  deco->request_mode.notify=decoration_handle_request_mode;
  wl_signal_add(&wlr_deco->events.request_mode,&deco->request_mode);
  deco->destroy.notify=decoration_handle_destroy;
  wl_signal_add(&wlr_deco->events.destroy,&deco->destroy);

  // wlr_xdg_toplevel_decoration_v1_set_mode(wlr_deco,WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);

  int width=xdg_surface->geometry.width;
  if(width<=0){
    width=400;
  }

  // Create a scene tree for the titlebar and buttons above the window surface
  deco->scene_tree=wlr_scene_tree_create(scene_tree);
  
  // Titlebar background(dark grey)
  deco->titlebar=wlr_scene_rect_create(deco->scene_tree,width,24,(float[]){0.25f,0.25f,0.25f,0.2f});
  wlr_scene_node_set_position(&deco->titlebar->node,0,-24);
  deco->titlebar->node.data=toplevel;

  // Close button(red)
  deco->close_button=wlr_scene_rect_create(deco->scene_tree,16,16,(float[]){0.8f,0.2f,0.2f,1.0f});
  wlr_scene_node_set_position(&deco->close_button->node,width-20,-20);
  deco->close_button->node.data=toplevel;
}
