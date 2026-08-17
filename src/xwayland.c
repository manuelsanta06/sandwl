#include <stdlib.h>

#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/xwayland.h>

#include <wlr/util/log.h>

#include "xwayland.h"
#include "types.h"

void server_xwayland_ready(struct wl_listener *listener,void *data){
  //Raised when XWayland is ready to accept connections
  wlr_log(WLR_ERROR,"Xwayland server running");
}

void xwayland_surface_map(struct wl_listener *listener,void *data){
  //Called when the X11 surface is ready to be displayed
  struct sandwl_xwayland_surface *surface=wl_container_of(listener,surface,map);
  
  //Link the X11 surface to the scene graph
  surface->scene_tree=wlr_scene_subsurface_tree_create(
    surface->server->scene_toplevel,surface->xwayland_surface->surface);
  surface->scene_tree->node.data=surface;
  
  wl_list_insert(&surface->server->toplevels,&surface->link);
  //TODO:focus the surface using existing focus logic
}

void xwayland_surface_unmap(struct wl_listener *listener,void *data){
  //Called when the X11 surface is hidden or closed
  struct sandwl_xwayland_surface *surface=wl_container_of(listener,surface,unmap);
  wl_list_remove(&surface->link);
  
  if(surface->scene_tree){
    wlr_scene_node_destroy(&surface->scene_tree->node);
    surface->scene_tree=NULL;
  }
}

void xwayland_surface_associate(struct wl_listener *listener,void *data){
  struct sandwl_xwayland_surface *surface=wl_container_of(listener,surface,associate);
  //Now xwayland_surface->surface is valid, listen to its map/unmap events
  surface->map.notify=xwayland_surface_map;
  wl_signal_add(&surface->xwayland_surface->surface->events.map,&surface->map);
  surface->unmap.notify=xwayland_surface_unmap;
  wl_signal_add(&surface->xwayland_surface->surface->events.unmap,&surface->unmap);
}

void xwayland_surface_dissociate(struct wl_listener *listener,void *data){
  struct sandwl_xwayland_surface *surface=wl_container_of(listener,surface,dissociate);
  wl_list_remove(&surface->map.link);
  wl_list_remove(&surface->unmap.link);
}

void xwayland_surface_destroy(struct wl_listener *listener,void *data){
  struct sandwl_xwayland_surface *surface=wl_container_of(listener,surface,destroy);
  wl_list_remove(&surface->associate.link);
  wl_list_remove(&surface->dissociate.link);
  wl_list_remove(&surface->destroy.link);
  wl_list_remove(&surface->request_configure.link);
  wl_list_remove(&surface->request_move.link);
  wl_list_remove(&surface->request_resize.link);
  free(surface);
}

void xwayland_surface_request_configure(struct wl_listener *listener,void *data){
  struct sandwl_xwayland_surface *surface=wl_container_of(listener,surface,request_configure);
  struct wlr_xwayland_surface_configure_event *event=data;
  wlr_xwayland_surface_configure(surface->xwayland_surface,event->x,event->y,event->width,event->height);
}

void xwayland_surface_request_move(struct wl_listener *listener,void *data){
  struct sandwl_xwayland_surface *surface=wl_container_of(listener,surface,request_move);
  //TODO: implement begin_interactive for XWayland similar to your xdg_toplevel logic
}

void xwayland_surface_request_resize(struct wl_listener *listener,void *data){
  struct sandwl_xwayland_surface *surface=wl_container_of(listener,surface,request_resize);
  struct wlr_xwayland_resize_event *event=data;
  //TODO: implement begin_interactive for XWayland resizing
}

void server_new_xwayland_surface(struct wl_listener *listener,void *data){
  struct sandwl_server *server=wl_container_of(listener,server,new_xwayland_surface);
  struct wlr_xwayland_surface *xwayland_surface=data;

  struct sandwl_xwayland_surface *surface=calloc(1,sizeof(*surface));
  surface->server=server;
  surface->xwayland_surface=xwayland_surface;

  //Listen to associate/dissociate instead of map/unmap directly
  surface->associate.notify=xwayland_surface_associate;
  wl_signal_add(&xwayland_surface->events.associate,&surface->associate);
  surface->dissociate.notify=xwayland_surface_dissociate;
  wl_signal_add(&xwayland_surface->events.dissociate,&surface->dissociate);

  surface->destroy.notify=xwayland_surface_destroy;
  wl_signal_add(&xwayland_surface->events.destroy,&surface->destroy);
  surface->request_configure.notify=xwayland_surface_request_configure;
  wl_signal_add(&xwayland_surface->events.request_configure,&surface->request_configure);

  surface->request_move.notify=xwayland_surface_request_move;
  wl_signal_add(&xwayland_surface->events.request_move,&surface->request_move);
  surface->request_resize.notify=xwayland_surface_request_resize;
  wl_signal_add(&xwayland_surface->events.request_resize,&surface->request_resize);
}
