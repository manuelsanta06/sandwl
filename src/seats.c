#include <wlr/types/wlr_primary_selection.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>

#include "types.h"
#include "seats.h"

void seat_request_cursor(struct wl_listener *listener,void *data){
  struct sandwl_server *server=wl_container_of(listener,server,request_cursor);
  //raised by the seat when a client provides a cursor image
  struct wlr_seat_pointer_request_set_cursor_event *event=data;
  struct wlr_seat_client *focused_client=server->seat->pointer_state.focused_client;
  //can be sent by any client,so check to make sure this one actually has pointer focus first
  if(focused_client==event->seat_client){
    wlr_cursor_set_surface(server->cursor,event->surface,event->hotspot_x,event->hotspot_y);
  }
}

void seat_pointer_focus_change(struct wl_listener *listener,void *data){
  struct sandwl_server *server=wl_container_of(listener,server,pointer_focus_change);
  //raised when the pointer focus is changed,including when the client is closed
  //set the cursor image to its default if target surface is NULL
  struct wlr_seat_pointer_focus_change_event *event=data;
  if(event->new_surface==NULL){
    wlr_cursor_set_xcursor(server->cursor,server->cursor_mgr,"default");
  }
}

void seat_request_set_selection(struct wl_listener *listener,void *data){
  //raised by the seat when a client wants to set the selection,usually when the user copies something
  struct sandwl_server *server=wl_container_of(listener,server,request_set_selection);
  struct wlr_seat_request_set_selection_event *event=data;
  wlr_seat_set_selection(server->seat,event->source,event->serial);
}

void seat_request_set_primary_selection(struct wl_listener *listener,void *data){
  struct sandwl_server *server=wl_container_of(listener,server,request_set_primary_selection);
  struct wlr_seat_request_set_primary_selection_event *event=data;
  wlr_seat_set_primary_selection(server->seat,event->source,event->serial);
}


void seat_request_start_drag(struct wl_listener *listener,void *data){
  struct sandwl_server *server=wl_container_of(listener,server,request_start_drag);
  struct wlr_seat_request_start_drag_event *event=data;

  if(wlr_seat_validate_pointer_grab_serial(server->seat,event->origin,event->serial))
    wlr_seat_start_pointer_drag(server->seat,event->drag,event->serial);
  else
    wlr_data_source_destroy(event->drag->source);
}

void seat_start_drag(struct wl_listener *listener,void *data){
  struct sandwl_server *server=wl_container_of(listener,server,start_drag);
  struct wlr_drag *drag=data;

  wlr_cursor_set_xcursor(server->cursor,server->cursor_mgr,"grabbing");

  if(drag->icon!=NULL){
    server->drag_icon=wlr_scene_drag_icon_create(server->scene_top,drag->icon);
    wlr_scene_node_set_position(&server->drag_icon->node,server->cursor->x,server->cursor->y);

    server->drag_icon_destroy.notify=seat_drag_icon_destroy;
    wl_signal_add(&drag->icon->events.destroy,&server->drag_icon_destroy);
  }
}


void seat_drag_icon_destroy(struct wl_listener *listener,void *data){
  struct sandwl_server *server=wl_container_of(listener,server,drag_icon_destroy);
  wl_list_remove(&server->drag_icon_destroy.link);
  server->drag_icon=NULL;
}
