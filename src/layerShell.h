#pragma once

#include <wlr/types/wlr_scene.h>


static void layer_surface_map(struct wl_listener *listener,void *data);
static void layer_surface_unmap(struct wl_listener *listener,void *data);
static void layer_surface_destroy(struct wl_listener *listener,void *data);
static void layer_surface_commit(struct wl_listener *listener,void *data);

void server_new_layer_surface(struct wl_listener *listener,void *data);
