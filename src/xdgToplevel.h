#pragma once

#include <wayland-server-core.h>

#include "types.h"


void xdg_toplevel_map(struct wl_listener *listener,void *data);

void xdg_toplevel_unmap(struct wl_listener *listener,void *data);

void xdg_toplevel_commit(struct wl_listener *listener,void *data);


//sets upp an interactive move/resize operation
//the compositor stops propagating pointer vents to clients and consumes them itself
void begin_interactive(struct sandwl_toplevel *toplevel,enum sandwl_cursor_mode mode,uint32_t edges);

void xdg_toplevel_request_move(struct wl_listener *listener,void *data);

void xdg_toplevel_request_resize(struct wl_listener *listener,void *data);

//TODO:
void xdg_toplevel_request_maximize(struct wl_listener *listener,void *data);

//TODO:
void xdg_toplevel_request_fullscreen(struct wl_listener *listener,void *data);

void xdg_toplevel_destroy(struct wl_listener *listener,void *data);

void server_new_xdg_toplevel(struct wl_listener *listener,void *data);

void server_new_xdg_popup(struct wl_listener *listener,void *data);
