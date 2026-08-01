#pragma once

#include "types.h"


void process_cursor_resize(struct sandwl_server *server);

void process_cursor_move(struct sandwl_server *server);

void process_cursor_motion(struct sandwl_server *server,uint32_t time);
