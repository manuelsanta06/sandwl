#pragma once

#include "types.h"


//sets upp an interactive move/resize operation
//the compositor stops propagating pointer vents to clients and consumes them itself
void begin_interactive(struct sandwl_server *server,struct wlr_scene_tree *tree,
    enum sandwl_cursor_mode mode,uint32_t edges,struct wlr_box geo_box);

void process_cursor_resize(struct sandwl_server *server);

void process_cursor_move(struct sandwl_server *server);

void process_cursor_motion(struct sandwl_server *server,uint32_t time);
