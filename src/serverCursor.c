#include <stdlib.h>

#include <wlr/types/wlr_pointer_constraints_v1.h>
#include <wlr/types/wlr_relative_pointer_v1.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>

#include "processCursor.h"
#include "serverCursor.h"
#include "utilies.h"
#include "types.h"


void server_cursor_motion(struct wl_listener *listener,void *data){
  //forwarded by the cursor when a pointer emits _relative_ pointer motion event(ej a delta)
  struct sandwl_server *server=wl_container_of(listener,server,cursor_motion);
  struct wlr_pointer_motion_event *event=data;
  wlr_relative_pointer_manager_v1_send_relative_motion(server->relative_pointer_manager,server->seat,
    (uint64_t)event->time_msec*1000,
    event->delta_x,event->delta_y,
    event->unaccel_dx,event->unaccel_dy
  );
  if(server->active_constraint&&server->active_constraint->type==WLR_POINTER_CONSTRAINT_V1_LOCKED){
    //cursor constraint
  }else{
    wlr_cursor_move(server->cursor,&event->pointer->base,event->delta_x,event->delta_y);
  }
  process_cursor_motion(server,event->time_msec);
  //update drag-and-drop icon position
  if(server->drag_icon!=NULL){
    wlr_scene_node_set_position(&server->drag_icon->node,server->cursor->x,server->cursor->y);
  }
}

void server_cursor_motion_absolute(struct wl_listener *listener,void *data){
  //this event is forwarded by the cursor when a pointer emits an _absolute_ motion event
  //from 0..1 on each axis
  //this happens when wlroots is running under a Wayland window rather than KMS+DRM
  //you could enter the window from any edge, so we have to warp the mouse there
  //there is also some hardware which
  //emits these events
  struct sandwl_server *server=wl_container_of(listener,server,cursor_motion_absolute);
  struct wlr_pointer_motion_absolute_event *event=data;
  if(server->active_constraint&&server->active_constraint->type==WLR_POINTER_CONSTRAINT_V1_LOCKED){
    //cursor constraint
  }else{
    wlr_cursor_warp_absolute(server->cursor,&event->pointer->base,event->x,event->y);
  }
  process_cursor_motion(server,event->time_msec);
}

void server_cursor_button(struct wl_listener *listener,void *data){
  //forwarded by the cursor when a pointer emits a button event
  struct sandwl_server *server=wl_container_of(listener,server,cursor_button);
  struct wlr_pointer_button_event *event=data;
  //notify the client with pointer focus
  wlr_seat_pointer_notify_button(server->seat,event->time_msec,event->button,event->state);
  if(event->state==WL_POINTER_BUTTON_STATE_RELEASED){
    //if button released exit interactive move/resize mode
    reset_cursor_mode(server);
  }else{
    //focus that client if the button was _pressed_
    double sx,sy;
    struct wlr_scene_node *node=wlr_scene_node_at(&server->scene->tree.node,
      server->cursor->x,server->cursor->y,&sx,&sy);

    if(node&&node->type==WLR_SCENE_NODE_RECT){
      //Handle clicks on server-side decorations
      struct sandwl_toplevel *toplevel=node->data;
      if(toplevel&&toplevel->decoration){
        focus_surface(server,toplevel->scene_tree,toplevel->xdg_toplevel->base->surface);
        if(node==&toplevel->decoration->close_button->node){
          wlr_xdg_toplevel_send_close(toplevel->xdg_toplevel);
          return;
        }else if(node==&toplevel->decoration->titlebar->node){
          begin_interactive(toplevel->server,toplevel->scene_tree,SANDWL_CURSOR_MOVE,0,(struct wlr_box){0});
          return;
        }
      }
    }else if(node&&node->type==WLR_SCENE_NODE_BUFFER){
      struct wlr_surface *root_surface=wlr_surface_get_root_surface(
        wlr_scene_surface_try_from_buffer(wlr_scene_buffer_from_node(node))->surface);
      struct wlr_layer_surface_v1 *layer=wlr_layer_surface_v1_try_from_wlr_surface(root_surface);
      if(layer&&layer->current.keyboard_interactive){
        struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(server->seat);
        if(keyboard){
          wlr_seat_keyboard_notify_enter(server->seat,root_surface,
            keyboard->keycodes,keyboard->num_keycodes,&keyboard->modifiers);
        }
      }else{
        struct wlr_surface *surface=NULL;
        struct wlr_scene_tree *node=desktop_tree_at(server,
          server->cursor->x,server->cursor->y,&surface,&sx,&sy);
        if(node)focus_surface(server,node,surface);
        else{
        }
      }
    }

    // struct sandwl_toplevel *toplevel=desktop_toplevel_at(server,
    //   server->cursor->x,server->cursor->y,&surface,&sx,&sy);
    // focus_toplevel(toplevel);
  }
}

void server_cursor_axis(struct wl_listener *listener,void *data){
  //forwarded by the cursor when a pointer emits an axis event
  //for example when you move the scroll wheel
  struct sandwl_server *server=wl_container_of(listener,server,cursor_axis);
  struct wlr_pointer_axis_event *event=data;
  //notify the client with pointer focus of the axis event
  wlr_seat_pointer_notify_axis(server->seat,
      event->time_msec,event->orientation,event->delta,
      event->delta_discrete,event->source,event->relative_direction);
}

void server_cursor_frame(struct wl_listener *listener,void *data){
  (void)data;
  //forwarded by the cursor whe a pointer emits an frame event
  struct sandwl_server *server=wl_container_of(listener,server,cursor_frame);
  //notify the client with pointer focus of the frame event
  wlr_seat_pointer_notify_frame(server->seat);
}

void server_new_pointer(struct sandwl_server *server,struct wlr_input_device *device){
  //pointer configuration should be applied here
  wlr_cursor_attach_input_device(server->cursor,device);
}

void handle_pointer_constraint_destroy(struct wl_listener *listener,void *data){
  (void)data;
  struct sandwl_pointer_constraint *sandwl_constraint=wl_container_of(listener,sandwl_constraint,destroy);
  struct sandwl_server *server=sandwl_constraint->server;

  if(server->active_constraint==sandwl_constraint->constraint){
    wlr_pointer_constraint_v1_send_deactivated(sandwl_constraint->constraint);
    server->active_constraint=NULL;
  }

  wl_list_remove(&sandwl_constraint->destroy.link);
  free(sandwl_constraint);
}

void handle_new_pointer_constraint(struct wl_listener *listener,void *data){
  struct sandwl_server *server=wl_container_of(listener,server,new_pointer_constraint);
  struct wlr_pointer_constraint_v1 *wlr_constraint=data;

  struct sandwl_pointer_constraint *sandwl_constraint=calloc(1,sizeof(*sandwl_constraint));
  sandwl_constraint->server=server;
  sandwl_constraint->constraint=wlr_constraint;

  sandwl_constraint->destroy.notify=handle_pointer_constraint_destroy;
  wl_signal_add(&wlr_constraint->events.destroy,&sandwl_constraint->destroy);

  if(server->seat->pointer_state.focused_surface==wlr_constraint->surface){
    server->active_constraint=wlr_constraint;
    wlr_pointer_constraint_v1_send_activated(wlr_constraint);
  }
}
