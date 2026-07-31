#pragma once

#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

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
