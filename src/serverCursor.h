#pragma once

#include <wayland-server-core.h>

#include "types.h"


void server_cursor_motion(struct wl_listener *listener,void *data);

void server_cursor_motion_absolute(struct wl_listener *listener,void *data);

void server_cursor_button(struct wl_listener *listener,void *data);

void server_cursor_axis(struct wl_listener *listener,void *data);

void server_cursor_frame(struct wl_listener *listener,void *data);

void server_new_pointer(struct sandwl_server *server,struct wlr_input_device *device);
