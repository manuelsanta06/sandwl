#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/util/edges.h>

#include "processCursor.h"
#include "utilies.h"
#include "types.h"


//sets upp an interactive move/resize operation
//the compositor stops propagating pointer vents to clients and consumes them itself
void begin_interactive(struct sandwl_server *server,struct wlr_scene_tree *tree,
    enum sandwl_cursor_mode mode,uint32_t edges,struct wlr_box geo_box){
  server->grabbed_tree=tree;
  server->cursor_mode=mode;

  if(mode==SANDWL_CURSOR_MOVE){
    server->grab_x=server->cursor->x-tree->node.x;
    server->grab_y=server->cursor->y-tree->node.y;
  }else if(mode==SANDWL_CURSOR_RESIZE){
    double borderx=(tree->node.x+geo_box.x)+((edges&WLR_EDGE_RIGHT)?geo_box.width:0);
    double bordery=(tree->node.y+geo_box.y)+((edges&WLR_EDGE_BOTTOM)?geo_box.height:0);
    server->grab_x=server->cursor->x-borderx;
    server->grab_y=server->cursor->y-bordery;

    server->grab_geobox=geo_box;
    server->grab_geobox.x+=tree->node.x;
    server->grab_geobox.y+=tree->node.y;

    server->resize_edges=edges;
  }
  if(!server->pointer_grab_active)
    wlr_seat_pointer_notify_clear_focus(server->seat);
}


void process_cursor_resize(struct sandwl_server *server){
  //TODO:resizing windows
}

void process_cursor_move(struct sandwl_server *server){
  wlr_scene_node_set_position(&server->grabbed_tree->node,
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

  if(server->pointer_grab_active){
    double sx=server->pointer_grab_sx+server->cursor->x-server->pointer_grab_x;
    double sy=server->pointer_grab_sy+server->cursor->y-server->pointer_grab_y;
    wlr_seat_pointer_notify_motion(server->seat,time,sx,sy);
    return;
  }

  //otherwise, find the toplevel under the pointer and send the event along
  double sx,sy;
  struct wlr_surface *surface=NULL;
  struct wlr_scene_tree *node=desktop_tree_at(server,server->cursor->x,
    server->cursor->y,&surface, &sx,&sy);
  if(!node){
    wlr_cursor_set_xcursor(server->cursor,server->cursor_mgr,"default");
  }
  if(surface){
    //gives the surface pointer focus
    wlr_seat_pointer_notify_enter(server->seat,surface,sx,sy);
    //gives the surface the pointer motion event
    wlr_seat_pointer_notify_motion(server->seat,time,sx,sy);
  }else{
    //clears pointer focus
    wlr_seat_pointer_notify_clear_focus(server->seat);
  }
}
