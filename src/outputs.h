#pragma once

#include <wayland-server-core.h>
#include "types.h"


void output_destroy(struct wl_listener *listener,void *data);

void output_request_state(struct wl_listener *listener,void *data);

void output_frame(struct wl_listener *listener,void *data);

void server_new_output(struct wl_listener *listener,void *data);

void arrange_layers(struct sandwl_output *output);
