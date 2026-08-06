#include <stdbool.h>
#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/edges.h>

#include "types.h"
#include "utilies.h"
#include "xdgToplevel.h"

void xdg_toplevel_map(struct wl_listener *listener,void *data){
  //called when surface is ready to display
  struct sandwl_toplevel *toplevel=wl_container_of(listener,toplevel,map);
  wl_list_insert(&toplevel->server->toplevels, &toplevel->link);
  focus_toplevel(toplevel);
}

void xdg_toplevel_unmap(struct wl_listener *listener,void *data){
  //Called when the surface is unmapped//hidden
  struct sandwl_toplevel *toplevel=wl_container_of(listener,toplevel,unmap);
  //Reset the cursor mode if the grabbed toplevel got unmapped
  if(toplevel==toplevel->server->grabbed_toplevel)
    reset_cursor_mode(toplevel->server);

  wl_list_remove(&toplevel->link);
}

void xdg_toplevel_commit(struct wl_listener *listener,void *data){
  //Called when a new surface state is committed
  struct sandwl_toplevel *toplevel=wl_container_of(listener,toplevel,commit);

  if(toplevel->xdg_toplevel->base->initial_commit){
    //the compositor must reply with a configure so the client can map the surface
    //configures the xdg_toplevel with 0,0 size to let the client pick the dimensions
    wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel,0,0);
  }
}


//sets upp an interactive move/resize operation
//the compositor stops propagating pointer vents to clients and consumes them itself
void begin_interactive(struct sandwl_toplevel *toplevel,enum sandwl_cursor_mode mode,uint32_t edges){
  struct sandwl_server *server=toplevel->server;

  server->grabbed_toplevel=toplevel;
  server->cursor_mode=mode;

  if(mode==SANDWL_CURSOR_MOVE){
    server->grab_x=server->cursor->x-toplevel->scene_tree->node.x;
    server->grab_y=server->cursor->y-toplevel->scene_tree->node.y;
  }else{
    struct wlr_box *geo_box=&toplevel->xdg_toplevel->base->geometry;

    double border_x=(toplevel->scene_tree->node.x+geo_box->x)+((edges&WLR_EDGE_RIGHT)?geo_box->width:0);
    double border_y=(toplevel->scene_tree->node.y+geo_box->y)+((edges&WLR_EDGE_BOTTOM)?geo_box->height:0);
    server->grab_x=server->cursor->x-border_x;
    server->grab_y=server->cursor->y-border_y;

    server->grab_geobox=*geo_box;
    server->grab_geobox.x+=toplevel->scene_tree->node.x;
    server->grab_geobox.y+=toplevel->scene_tree->node.y;

    server->resize_edges=edges;
  }
}

void xdg_toplevel_request_move(struct wl_listener *listener,void *data){
  //raised when a client would like to begin an interactive move
  //TODO: prevent the client from requesting this whenever they want
  struct sandwl_toplevel *toplevel=wl_container_of(listener,toplevel,request_move);
  begin_interactive(toplevel,SANDWL_CURSOR_MOVE,0);
}

void xdg_toplevel_request_resize(struct wl_listener *listener,void *data){
  //raised when a client would like to begin an interactive resize
  //TODO: prevent the client from requesting this whenever they want
  struct wlr_xdg_toplevel_resize_event *event=data;
  struct sandwl_toplevel *toplevel=wl_container_of(listener,toplevel,request_resize);
  begin_interactive(toplevel,SANDWL_CURSOR_RESIZE,event->edges);
}

//TODO:
void xdg_toplevel_request_maximize(struct wl_listener *listener,void *data){
  //raised when a client would like to maximize
  struct sandwl_toplevel *toplevel=wl_container_of(listener,toplevel,request_maximize);
  if(toplevel->xdg_toplevel->base->initialized){
    wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
  }
}

//TODO:
void xdg_toplevel_request_fullscreen(struct wl_listener *listener,void *data){
  //raised when a client would like to fullscreen
  struct sandwl_toplevel *toplevel=wl_container_of(listener,toplevel,request_fullscreen);
  if(toplevel->xdg_toplevel->base->initialized){
    wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
  }
}

void xdg_toplevel_destroy(struct wl_listener *listener,void *data){
  //Called when the xdg_toplevel is destroyed
  struct sandwl_toplevel *toplevel=wl_container_of(listener,toplevel,destroy);

  wl_list_remove(&toplevel->map.link);
  wl_list_remove(&toplevel->unmap.link);
  wl_list_remove(&toplevel->commit.link);
  wl_list_remove(&toplevel->destroy.link);
  wl_list_remove(&toplevel->request_move.link);
  wl_list_remove(&toplevel->request_resize.link);
  wl_list_remove(&toplevel->request_maximize.link);
  wl_list_remove(&toplevel->request_fullscreen.link);

  free(toplevel);
}

void server_new_xdg_toplevel(struct wl_listener *listener,void *data){
  //raised when a client creates a new toplevel/application window
  struct sandwl_server *server=wl_container_of(listener,server,new_xdg_toplevel);
  struct wlr_xdg_toplevel *xdg_toplevel=data;

  struct sandwl_toplevel *toplevel=calloc(1,sizeof(*toplevel));
  toplevel->server=server;
  toplevel->xdg_toplevel=xdg_toplevel;
  toplevel->scene_tree=wlr_scene_xdg_surface_create(toplevel->server->scene_toplevel,xdg_toplevel->base);
  toplevel->scene_tree->node.data=toplevel;
  xdg_toplevel->base->data=toplevel->scene_tree;

  //Listen to most of events it can emit
  toplevel->map.notify=xdg_toplevel_map;
  wl_signal_add(&xdg_toplevel->base->surface->events.map,&toplevel->map);
  toplevel->unmap.notify=xdg_toplevel_unmap;
  wl_signal_add(&xdg_toplevel->base->surface->events.unmap,&toplevel->unmap);
  toplevel->commit.notify=xdg_toplevel_commit;
  wl_signal_add(&xdg_toplevel->base->surface->events.commit,&toplevel->commit);

  toplevel->destroy.notify = xdg_toplevel_destroy;
  wl_signal_add(&xdg_toplevel->events.destroy, &toplevel->destroy);

  toplevel->request_move.notify=xdg_toplevel_request_move;
  wl_signal_add(&xdg_toplevel->events.request_move,&toplevel->request_move);
  toplevel->request_resize.notify=xdg_toplevel_request_resize;
  wl_signal_add(&xdg_toplevel->events.request_resize,&toplevel->request_resize);
  toplevel->request_maximize.notify=xdg_toplevel_request_maximize;
  wl_signal_add(&xdg_toplevel->events.request_maximize,&toplevel->request_maximize);
  toplevel->request_fullscreen.notify=xdg_toplevel_request_fullscreen;
  wl_signal_add(&xdg_toplevel->events.request_fullscreen,&toplevel->request_fullscreen);
}

void server_new_xdg_popup(struct wl_listener *listener,void *data){
  //Called when a new surface state is committed
  struct sandwl_popup *popup=wl_container_of(listener,popup,commit);
  struct wlr_xdg_popup *xdg_popup=data;
  struct wlr_xdg_surface *parent=wlr_xdg_surface_try_from_wlr_surface(xdg_popup->parent);

  if(!parent)return;

  if(popup->xdg_popup->base->initial_commit){
    //When an xdg_surface performs an initial commit, the compositor must
    //reply with a configure so the client can map the surface
    //currently sends an empty configure. A more sophisticated compositor
    //should change an xdg_popup's geometry to ensure it's not positioned
    //off-screen, for example
    wlr_xdg_surface_schedule_configure(popup->xdg_popup->base);
  }
}
