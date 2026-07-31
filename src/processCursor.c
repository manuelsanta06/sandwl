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
#include "utilies.h"
#include "processCursor.h"


void process_cursor_resize(struct sandwl_server *server){
  //TODO:resizing windows
}

void process_cursor_move(struct sandwl_server *server){
  struct sandwl_toplevel *toplevel=server->grabbed_toplevel;
  wlr_scene_node_set_position(&toplevel->scene_tree->node,
    server->cursor->x-server->grab_x,server->cursor->y-server->grab_y);
}

void process_cursor_motion(struct sandwl_server *server,uint32_t time){
  //if the mode is non-passthrough, delegate to those functions
  if(server->cursor_mode==SANDWL_CURSOR_MOVE){
    process_cursor_move(server);
    return;
  }else if(server->cursor_mode==SANDWL_CURSOR_RESIZE){
    process_cursor_resize(server);
    return;
  }

  //otherwise, find the toplevel under the pointer and send the event along
  double sx,sy;
  struct wlr_surface *surface=NULL;
  struct sandwl_toplevel *toplevel=desktop_toplevel_at(server,server->cursor->x,
    server->cursor->y,&surface, &sx,&sy);
  if(!toplevel){
    wlr_cursor_set_xcursor(server->cursor,server->cursor_mgr,"default");
  }
  if(surface){
    //gives the surface pointer focus
    wlr_seat_pointer_notify_enter(server->seat,surface,sx,sy);
    //gives the surface the pointer motion event
    wlr_seat_pointer_notify_motion(server->seat,time,sx,sy);
  }else{
    //clears pointer focus
    wlr_seat_pointer_clear_focus(server->seat);
  }
}
