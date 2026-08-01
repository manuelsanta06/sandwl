#pragma once

#include <wlr/types/wlr_compositor.h>

#include "types.h"


void focus_toplevel(struct sandwl_toplevel *toplevel);

void reset_cursor_mode(struct sandwl_server *server);

//return the topmost node in the scene at a given layout coords
struct sandwl_toplevel *desktop_toplevel_at(
  struct sandwl_server *server,double lx,double ly,
  struct wlr_surface **surface,double*sx,double*sy);
