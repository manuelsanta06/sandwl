#include <wlr/types/wlr_xdg_decoration_v1.h>

#include <stdlib.h>

#include "types.h"


void decoration_handle_destroy(struct wl_listener *listener,void *data){
  struct sandwl_decoration *deco=wl_container_of(listener,deco,destroy);
  wl_list_remove(&deco->request_mode.link);
  wl_list_remove(&deco->destroy.link);
  free(deco);
}

void decoration_handle_request_mode(struct wl_listener *listener,void *data){
  struct sandwl_decoration *deco=wl_container_of(listener,deco,request_mode);

  if (deco->wlr_decoration->toplevel->base->initialized){
    wlr_xdg_toplevel_decoration_v1_set_mode(deco->wlr_decoration,WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
  }
}

void server_new_xdg_decoration(struct wl_listener *listener,void *data){
  struct wlr_xdg_toplevel_decoration_v1 *wlr_deco=data;

  struct sandwl_decoration *deco=calloc(1,sizeof(*deco));
  deco->wlr_decoration=wlr_deco;

  deco->request_mode.notify=decoration_handle_request_mode;
  wl_signal_add(&wlr_deco->events.request_mode,&deco->request_mode);

  deco->destroy.notify=decoration_handle_destroy;
  wl_signal_add(&wlr_deco->events.destroy,&deco->destroy);
}
