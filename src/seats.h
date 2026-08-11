#pragma once

#include <wayland-server-core.h>


void seat_request_cursor(struct wl_listener *listener,void *data);

void seat_pointer_focus_change(struct wl_listener *listener,void *data);

void seat_request_set_selection(struct wl_listener *listener,void *data);

void seat_request_set_primary_selection(struct wl_listener *listener,void *data);


void seat_request_start_drag(struct wl_listener *listener,void *data);

void seat_start_drag(struct wl_listener *listener,void *data);

void seat_drag_icon_destroy(struct wl_listener *listener,void *data);
