#pragma once

#include <wayland-server-core.h>


void popup_destroy(struct wl_listener *listener, void *data);
void popup_commit(struct wl_listener *listener, void *data);
void popup_reposition(struct wl_listener *listener,void *data);

void layer_surface_new_popup(struct wl_listener *listener, void *data);
void layer_surface_map(struct wl_listener *listener,void *data);
void layer_surface_unmap(struct wl_listener *listener,void *data);
void layer_surface_destroy(struct wl_listener *listener,void *data);
void layer_surface_commit(struct wl_listener *listener,void *data);

void server_new_layer_surface(struct wl_listener *listener,void *data);
