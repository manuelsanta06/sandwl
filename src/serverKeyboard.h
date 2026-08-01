#pragma once

#include <wayland-server-core.h>
#include <wlr/types/wlr_input_device.h>

#include "types.h"


void keyboard_handle_modifiers(struct wl_listener *listener,void *data);

void keyboard_handle_key(struct wl_listener *listener,void *data);

void keyboard_handle_destroy(struct wl_listener *listener,void *data);

void server_new_keyboard(struct sandwl_server *server,struct wlr_input_device *device);
