#pragma once

#include <wlr/types/wlr_compositor.h>

#include "types.h"


void focus_surface(struct sandwl_server *server,struct wlr_scene_tree *tree,struct wlr_surface *surface);

void reset_cursor_mode(struct sandwl_server *server);

//return the topmost node in the scene at a given layout coords
struct wlr_scene_tree *desktop_tree_at(
  struct sandwl_server *server,double lx,double ly,
  struct wlr_surface **surface,double*sx,double*sy);
