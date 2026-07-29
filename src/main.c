#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
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


enum sandwl_cursor_mode {
  SANDWL_CURSOR_PASSTHROUGH,
  SANDWL_CURSOR_MOVE,
  SANDWL_CURSOR_RESIZE,
};

struct sandwl_server{
  struct wl_display               *wl_display;
  struct wlr_backend              *backend;
  struct wlr_renderer             *renderer;
  struct wlr_allocator            *allocator;
  struct wlr_scene                *scene;
  struct wlr_scene_output_layout  *scene_layout;

  struct wlr_xdg_shell            *xdg_shell;
  struct wl_listener              new_xdg_toplevel;
  struct wl_listener              new_xdg_popup;
  struct wl_list                  toplevels;

  struct wlr_cursor               *cursor;
  struct wlr_xcursor_manager      *cursor_mgr;
  struct wl_listener              cursor_motion;
  struct wl_listener              cursor_motion_absolute;
  struct wl_listener              cursor_button;
  struct wl_listener              cursor_axis;
  struct wl_listener              cursor_frame;


  struct wlr_seat                 *seat;
  struct wl_listener              new_input;
  struct wl_listener              request_cursor;
  struct wl_listener              pointer_focus_change;
  struct wl_listener              request_set_selection;
  struct wl_list                  keyboards;
  enum sandwl_cursor_mode         cursor_mode;
  struct sandwl_toplevel          *grabbed_toplevel;
  double grab_x, grab_y;
  struct wlr_box                  grab_geobox;
  uint32_t                        resize_edges;

  struct wl_event_loop            *wl_event_loop;

  struct wlr_output_layout        *output_layout;
  struct wl_list                  outputs;
  struct wl_listener              new_output;
};

struct sandwl_output{
  struct wl_list        link;
  struct sandwl_server  *server;
  struct wlr_output     *wlr_output;
  struct wl_listener    frame;
  struct wl_listener    request_state;
  struct wl_listener    destroy;
};

struct sandwl_toplevel{
  struct wl_list          link;
  struct sandwl_server    *server;
  struct wlr_xdg_toplevel *xdg_toplevel;
  struct wlr_scene_tree   *scene_tree;
  struct wl_listener      map;
  struct wl_listener      unmap;
  struct wl_listener      commit;
  struct wl_listener      destroy;
  struct wl_listener      request_move;
  struct wl_listener      request_resize;
  struct wl_listener      request_maximize;
  struct wl_listener      request_fullscreen;
};

struct sandwl_popup{
  struct wlr_xdg_popup    *xdg_popup;
  struct wl_listener      commit;
  struct wl_listener      destroy;
};

struct sandwl_keyboard{
  struct wl_list          link;
  struct sandwl_server    *server;
  struct wlr_keyboard     *wlr_keyboard;

  struct wl_listener      modifiers;
  struct wl_listener      key;
  struct wl_listener      destroy;
};


static void output_destroy(struct wl_listener *listener,void *data){
  struct sandwl_output *output=wl_container_of(listener,output,destroy);

  wl_list_remove(&output->frame.link);
  wl_list_remove(&output->request_state.link);
  wl_list_remove(&output->destroy.link);
  wl_list_remove(&output->link);
  free(output);
}

static void output_request_state(struct wl_listener *listener,void *data){
  // This function is called when the backend requests a new state for
  // the output. For example, Wayland and X11 backends request a new mode
  // when the output window is resized.
  struct sandwl_output *output=wl_container_of(listener,output,request_state);
  const struct wlr_output_event_request_state *event=data;
  wlr_output_commit_state(output->wlr_output,event->state);
}

static void output_frame(struct wl_listener *listener,void *data){
  //runs each time an output wants to render a frame
  struct sandwl_output *output=wl_container_of(listener,output,frame);
  struct wlr_scene *scene=output->server->scene;

  struct wlr_scene_output *scene_output=wlr_scene_get_scene_output(
    scene,output->wlr_output);

  //Render the scene if needed and commit the output
  wlr_scene_output_commit(scene_output,NULL);

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC,&now);
  wlr_scene_output_send_frame_done(scene_output,&now);
}

static void server_new_output(struct wl_listener *listener,void *data){
  struct sandwl_server *server=wl_container_of(listener,server,new_output);
  struct wlr_output *wlr_output=data;

  wlr_output_init_render(wlr_output,server->allocator,server->renderer);

  struct wlr_output_state state;
  wlr_output_state_init(&state);
  wlr_output_state_set_enabled(&state,true);

  //set output mode(size and refresh rate)
  struct wlr_output_mode *mode=wlr_output_preferred_mode(wlr_output);
  if(mode!=NULL){
    wlr_output_state_set_mode(&state,mode);
  }

  wlr_output_commit_state(wlr_output, &state);
  wlr_output_state_finish(&state);

  struct sandwl_output *output=calloc(1,sizeof(*output));
  output->wlr_output=wlr_output;
  output->server=server;

  output->frame.notify=output_frame;
  wl_signal_add(&wlr_output->events.frame,&output->frame);
  output->request_state.notify=output_request_state;
  wl_signal_add(&wlr_output->events.request_state,&output->request_state);
  output->destroy.notify=output_destroy;
  wl_signal_add(&wlr_output->events.destroy,&output->destroy);

  wl_list_insert(&server->outputs,&output->link);

  //adds the output to the layout
  //'wlr_output_layout_add_auto' arranges outputs from left to right
  struct wlr_output_layout_output *l_output=wlr_output_layout_add_auto(
    server->output_layout,wlr_output);
  struct wlr_scene_output *scene_output=wlr_scene_output_create(server->scene,wlr_output);
  wlr_scene_output_layout_add_output(server->scene_layout,l_output,scene_output);
}


static void focus_toplevel(struct sandwl_toplevel *toplevel){
  /* Note: this function only deals with keyboard focus. */
  if(toplevel==NULL){
    return;
  }
  struct sandwl_server *server=toplevel->server;
  struct wlr_seat *seat=server->seat;
  struct wlr_surface *prev_surface=seat->keyboard_state.focused_surface;
  struct wlr_surface *surface=toplevel->xdg_toplevel->base->surface;
  if(prev_surface==surface)return;

  if(prev_surface){
    //Deactivate the previously focused surface
    struct wlr_xdg_toplevel *prev_toplevel=
      wlr_xdg_toplevel_try_from_wlr_surface(prev_surface);
    if(prev_toplevel!=NULL){
      wlr_xdg_toplevel_set_activated(prev_toplevel,false);
    }
  }
  struct wlr_keyboard *keyboard=wlr_seat_get_keyboard(seat);
  //Move the toplevel to the front
  wlr_scene_node_raise_to_top(&toplevel->scene_tree->node);
  wl_list_remove(&toplevel->link);
  wl_list_insert(&server->toplevels,&toplevel->link);
  //Activate the new surface
  wlr_xdg_toplevel_set_activated(toplevel->xdg_toplevel,true);
  //Tell the seat to have the keyboard enter this surface
  if(keyboard!=NULL){
    wlr_seat_keyboard_notify_enter(seat,surface,
      keyboard->keycodes,keyboard->num_keycodes,&keyboard->modifiers);
  }
}

static void reset_cursor_mode(struct sandwl_server *server){
  //Reset the cursor mode to passthrough
  server->cursor_mode=SANDWL_CURSOR_PASSTHROUGH;
  server->grabbed_toplevel=NULL;
}


static void xdg_toplevel_map(struct wl_listener *listener,void *data){
  //called when surface is ready to display
  struct sandwl_toplevel *toplevel=wl_container_of(listener,toplevel,map);
  wl_list_insert(&toplevel->server->toplevels, &toplevel->link);
  focus_toplevel(toplevel);
}

static void xdg_toplevel_unmap(struct wl_listener *listener,void *data){
  //Called when the surface is unmapped//hidden
  struct sandwl_toplevel *toplevel=wl_container_of(listener,toplevel,unmap);
  //Reset the cursor mode if the grabbed toplevel got unmapped
  if(toplevel==toplevel->server->grabbed_toplevel)
    reset_cursor_mode(toplevel->server);

  wl_list_remove(&toplevel->link);
}

static void xdg_toplevel_commit(struct wl_listener *listener,void *data){
  //Called when a new surface state is committed
  struct sandwl_toplevel *toplevel=wl_container_of(listener,toplevel,commit);

  if(toplevel->xdg_toplevel->base->initial_commit){
    //the compositor must reply with a configure so the client can map the surface
    //configures the xdg_toplevel with 0,0 size to let the client pick the dimensions
    wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel,0,0);
  }
}


//sets upp an interactive move/resize operation
//the compositor stops propagating pointer vents to clients and consumes them itself
static void begin_interactive(struct sandwl_toplevel *toplevel,enum sandwl_cursor_mode mode,uint32_t edges){
  struct sandwl_server *server=toplevel->server;

  server->grabbed_toplevel=toplevel;
  server->cursor_mode=mode;

  if(mode==SANDWL_CURSOR_MOVE){
    server->grab_x=server->cursor->x-toplevel->scene_tree->node.x;
    server->grab_y=server->cursor->y-toplevel->scene_tree->node.y;
  }else{
    struct wlr_box *geo_box=&toplevel->xdg_toplevel->base->geometry;

    double border_x=(toplevel->scene_tree->node.x+geo_box->x)+((edges&WLR_EDGE_RIGHT)?geo_box->width:0);
    double border_y=(toplevel->scene_tree->node.y+geo_box->y)+((edges&WLR_EDGE_BOTTOM)?geo_box->height:0);
    server->grab_x=server->cursor->x-border_x;
    server->grab_y=server->cursor->y-border_y;

    server->grab_geobox=*geo_box;
    server->grab_geobox.x+=toplevel->scene_tree->node.x;
    server->grab_geobox.y+=toplevel->scene_tree->node.y;

    server->resize_edges=edges;
  }
}

static void xdg_toplevel_request_move(struct wl_listener *listener,void *data){
  //raised when a client would like to begin an interactive move
  //TODO: prevent the client from requesting this whenever they want
  struct sandwl_toplevel *toplevel=wl_container_of(listener,toplevel,request_move);
  begin_interactive(toplevel,SANDWL_CURSOR_MOVE,0);
}

static void xdg_toplevel_request_resize(struct wl_listener *listener,void *data){
  //raised when a client would like to begin an interactive resize
  //TODO: prevent the client from requesting this whenever they want
  struct wlr_xdg_toplevel_resize_event *event=data;
  struct sandwl_toplevel *toplevel=wl_container_of(listener,toplevel,request_resize);
  begin_interactive(toplevel,SANDWL_CURSOR_RESIZE,event->edges);
}

//TODO:
static void xdg_toplevel_request_maximize(struct wl_listener *listener,void *data){
  //raised when a client would like to maximize
  struct sandwl_toplevel *toplevel=wl_container_of(listener,toplevel,request_maximize);
  if(toplevel->xdg_toplevel->base->initialized){
    wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
  }
}

//TODO:
static void xdg_toplevel_request_fullscreen(struct wl_listener *listener,void *data){
  //raised when a client would like to fullscreen
  struct sandwl_toplevel *toplevel=wl_container_of(listener,toplevel,request_fullscreen);
  if(toplevel->xdg_toplevel->base->initialized){
    wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
  }
}

static void xdg_toplevel_destroy(struct wl_listener *listener,void *data){
  //Called when the xdg_toplevel is destroyed
  struct sandwl_toplevel *toplevel=wl_container_of(listener,toplevel,destroy);

  wl_list_remove(&toplevel->map.link);
  wl_list_remove(&toplevel->unmap.link);
  wl_list_remove(&toplevel->commit.link);
  wl_list_remove(&toplevel->destroy.link);
  wl_list_remove(&toplevel->request_move.link);
  wl_list_remove(&toplevel->request_resize.link);
  wl_list_remove(&toplevel->request_maximize.link);
  wl_list_remove(&toplevel->request_fullscreen.link);

  free(toplevel);
}

static void server_new_xdg_toplevel(struct wl_listener *listener,void *data){
  //raised when a client creates a new toplevel/application window
  struct sandwl_server *server=wl_container_of(listener,server,new_xdg_toplevel);
  struct wlr_xdg_toplevel *xdg_toplevel=data;

  struct sandwl_toplevel *toplevel=calloc(1,sizeof(*toplevel));
  toplevel->server=server;
  toplevel->xdg_toplevel=xdg_toplevel;
  toplevel->scene_tree=
    wlr_scene_xdg_surface_create(&toplevel->server->scene->tree,xdg_toplevel->base);
  toplevel->scene_tree->node.data=toplevel;
  xdg_toplevel->base->data=toplevel->scene_tree;

  //Listen to most of events it can emit
  toplevel->map.notify=xdg_toplevel_map;
  wl_signal_add(&xdg_toplevel->base->surface->events.map,&toplevel->map);
  toplevel->unmap.notify=xdg_toplevel_unmap;
  wl_signal_add(&xdg_toplevel->base->surface->events.unmap,&toplevel->unmap);
  toplevel->commit.notify=xdg_toplevel_commit;
  wl_signal_add(&xdg_toplevel->base->surface->events.commit,&toplevel->commit);

  toplevel->destroy.notify = xdg_toplevel_destroy;
  wl_signal_add(&xdg_toplevel->events.destroy, &toplevel->destroy);

  toplevel->request_move.notify = xdg_toplevel_request_move;
  wl_signal_add(&xdg_toplevel->events.request_move, &toplevel->request_move);
  toplevel->request_resize.notify = xdg_toplevel_request_resize;
  wl_signal_add(&xdg_toplevel->events.request_resize, &toplevel->request_resize);
  toplevel->request_maximize.notify = xdg_toplevel_request_maximize;
  wl_signal_add(&xdg_toplevel->events.request_maximize, &toplevel->request_maximize);
  toplevel->request_fullscreen.notify = xdg_toplevel_request_fullscreen;
  wl_signal_add(&xdg_toplevel->events.request_fullscreen, &toplevel->request_fullscreen);
}

static void server_new_xdg_popup(struct wl_listener *listener,void *data){
  //Called when a new surface state is committed
  struct sandwl_popup *popup=wl_container_of(listener,popup,commit);
  if(popup->xdg_popup->base->initial_commit){
    //When an xdg_surface performs an initial commit, the compositor must
    //reply with a configure so the client can map the surface
    //currently sends an empty configure. A more sophisticated compositor
    //should change an xdg_popup's geometry to ensure it's not positioned
    //off-screen, for example
    wlr_xdg_surface_schedule_configure(popup->xdg_popup->base);
  }
}


//return the topmost node in the scene at a given layout coords
static struct sandwl_toplevel *desktop_toplevel_at(
  struct sandwl_server *server,double lx,double ly,
  struct wlr_surface **surface,double*sx,double*sy){
  struct wlr_scene_node *node=wlr_scene_node_at(&server->scene->tree.node,lx,ly,sx,sy);
  if(node==NULL||node->type!=WLR_SCENE_NODE_BUFFER){
    return NULL;
  }
  struct wlr_scene_buffer *scene_buffer=wlr_scene_buffer_from_node(node);
  struct wlr_scene_surface *scene_surface=wlr_scene_surface_try_from_buffer(scene_buffer);
  if(!scene_surface){
    return NULL;
  }

  *surface=scene_surface->surface;
  //find the node corresponding to the sandwl_toplevel at the root of this surface tree
  struct wlr_scene_tree *tree=node->parent;
  while(tree!=NULL&&tree->node.data==NULL){
    tree=tree->node.parent;
  }
  return tree->node.data;
}


static void process_cursor_resize(struct sandwl_server *server){
  //TODO:resizing windows
}

static void process_cursor_move(struct sandwl_server *server){
  struct sandwl_toplevel *toplevel=server->grabbed_toplevel;
  wlr_scene_node_set_position(&toplevel->scene_tree->node,
    server->cursor->x-server->grab_x,server->cursor->y-server->grab_y);
}

static void process_cursor_motion(struct sandwl_server *server,uint32_t time){
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

static void server_cursor_motion(struct wl_listener *listener,void *data){
  //forwarded by the cursor when a pointer emits _relative_ pointer motion event(ej a delta)
  struct sandwl_server *server=wl_container_of(listener,server,cursor_motion);
  struct wlr_pointer_motion_event *event=data;
  wlr_cursor_move(server->cursor,&event->pointer->base,event->delta_x,event->delta_y);
  process_cursor_motion(server,event->time_msec);
}

static void server_cursor_motion_absolute(struct wl_listener *listener,void *data){
  //this event is forwarded by the cursor when a pointer emits an _absolute_ motion event
  //from 0..1 on each axis
  //this happens when wlroots is running under a Wayland window rather than KMS+DRM
  //you could enter the window from any edge, so we have to warp the mouse there
  //there is also some hardware which
  //emits these events
  struct sandwl_server *server=wl_container_of(listener,server,cursor_motion_absolute);
  struct wlr_pointer_motion_absolute_event *event=data;
  wlr_cursor_warp_absolute(server->cursor,&event->pointer->base,event->x,event->y);
  process_cursor_motion(server,event->time_msec);
}

static void server_cursor_button(struct wl_listener *listener,void *data){
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
    struct wlr_surface *surface=NULL;
    struct sandwl_toplevel *toplevel=desktop_toplevel_at(server,
      server->cursor->x,server->cursor->y,&surface,&sx,&sy);
    focus_toplevel(toplevel);
  }
}

static void server_cursor_axis(struct wl_listener *listener,void *data){
  //forwarded by the cursor when a pointer emits an axis event
  //for example when you move the scroll wheel
  struct sandwl_server *server=wl_container_of(listener,server,cursor_axis);
  struct wlr_pointer_axis_event *event=data;
  //notify the client with pointer focus of the axis event
  wlr_seat_pointer_notify_axis(server->seat,
      event->time_msec,event->orientation,event->delta,
      event->delta_discrete,event->source,event->relative_direction);
}

static void server_cursor_frame(struct wl_listener *listener,void *data){
  //forwarded by the cursor whe a pointer emits an frame event
  struct sandwl_server *server=wl_container_of(listener,server,cursor_frame);
  //notify the client with pointer focus of the frame event
  wlr_seat_pointer_notify_frame(server->seat);
}



static void keyboard_handle_modifiers(struct wl_listener *listener,void *data){
  //raised on modifier pres
  struct sandwl_keyboard *keyboard=wl_container_of(listener,keyboard,modifiers);
  wlr_seat_set_keyboard(keyboard->server->seat,keyboard->wlr_keyboard);
  //pass modifier to client
  wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,&keyboard->wlr_keyboard->modifiers);
}

static void keyboard_handle_key(struct wl_listener *listener,void *data){
  //raised on key pres
  struct sandwl_keyboard *keyboard=wl_container_of(listener,keyboard,key);
  struct sandwl_server *server=keyboard->server;
  struct wlr_keyboard_key_event *event=data;
  struct wlr_seat *seat=server->seat;

  //Translate from libinput keycode to xkbcommon
  uint32_t keycode=event->keycode+8;
  //Get a list of keysyms based on the keymap for this keyboard
  const xkb_keysym_t *syms;
  int nsyms=xkb_state_key_get_syms(keyboard->wlr_keyboard->xkb_state,keycode,&syms);

  bool handled=false;
  uint32_t modifiers=wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);
  if(false){
    //handle system keybindings here, before giving them to toplevels
    //set 'handled' to true if keys get used
  }
  //examble
  if((modifiers&WLR_MODIFIER_ALT)&&event->state==WL_KEYBOARD_KEY_STATE_PRESSED){
    for (int i=0;i<nsyms;i++){
      switch(syms[i]){
        case XKB_KEY_q:
          if(fork()==0)
            execl("/bin/sh","/bin/sh","-c","kitty",(void*)NULL);
          break;
        default:
          break;
      }
    }
  }
  if(!handled){
    //pass keys to client
    wlr_seat_set_keyboard(seat,keyboard->wlr_keyboard);
    wlr_seat_keyboard_notify_key(seat,event->time_msec,event->keycode,event->state);
  }
}
static void keyboard_handle_destroy(struct wl_listener *listener,void *data){
  struct sandwl_keyboard *keyboard=wl_container_of(listener,keyboard,destroy);
  wl_list_remove(&keyboard->modifiers.link);
  wl_list_remove(&keyboard->key.link);
  wl_list_remove(&keyboard->destroy.link);
  wl_list_remove(&keyboard->link);
  free(keyboard);
}

static void server_new_keyboard(struct sandwl_server *server,struct wlr_input_device *device){
  struct wlr_keyboard *wlr_keyboard=wlr_keyboard_from_input_device(device);
  struct sandwl_keyboard *keyboard=calloc(1,sizeof(*keyboard));
  keyboard->server=server;
  keyboard->wlr_keyboard=wlr_keyboard;

  //prepare an XKB keymap and assign it to the keyboard. assumes default layout
  struct xkb_context *context=xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  struct xkb_keymap *keymap=xkb_keymap_new_from_names(context,NULL,XKB_KEYMAP_COMPILE_NO_FLAGS);

  wlr_keyboard_set_keymap(wlr_keyboard,keymap);
  xkb_keymap_unref(keymap);
  xkb_context_unref(context);
  wlr_keyboard_set_repeat_info(wlr_keyboard,25,600);

  //listeners for keyboard events
  keyboard->modifiers.notify=keyboard_handle_modifiers;
  wl_signal_add(&wlr_keyboard->events.modifiers,&keyboard->modifiers);
  keyboard->key.notify=keyboard_handle_key;
  wl_signal_add(&wlr_keyboard->events.key,&keyboard->key);
  keyboard->destroy.notify=keyboard_handle_destroy;
  wl_signal_add(&device->events.destroy, &keyboard->destroy);

  wlr_seat_set_keyboard(server->seat,keyboard->wlr_keyboard);

  //add the keyboard to list of keyboards
  wl_list_insert(&server->keyboards,&keyboard->link);
}

static void server_new_pointer(struct sandwl_server *server,struct wlr_input_device *device){
  //pointer configuration should be applied here
  wlr_cursor_attach_input_device(server->cursor,device);
}

static void server_new_input(struct wl_listener *listener,void *data){
  //raised when a new input becomes available
  struct sandwl_server *server=wl_container_of(listener,server,new_input);
  struct wlr_input_device *device=data;
  switch(device->type){
    case WLR_INPUT_DEVICE_KEYBOARD:
      server_new_keyboard(server,device);
      break;
    case WLR_INPUT_DEVICE_POINTER:
      server_new_pointer(server,device);
      break;
    // case WLR_INPUT_DEVICE_SWITCH:
    //   break;
    // case WLR_INPUT_DEVICE_TABLET:
    //   break;
    // case WLR_INPUT_DEVICE_TABLET_PAD:
    //   break;
    // case WLR_INPUT_DEVICE_TOUCH:
    //   break;
    default:
      break;
  }
  uint32_t caps=WL_SEAT_CAPABILITY_POINTER;
  if(!wl_list_empty(&server->keyboards)){
    caps|=WL_SEAT_CAPABILITY_KEYBOARD;
  }
  wlr_seat_set_capabilities(server->seat,caps);
}

static void seat_request_cursor(struct wl_listener *listener,void *data){
  struct sandwl_server *server=wl_container_of(listener,server,request_cursor);
  //raised by the seat when a client provides a cursor image
  struct wlr_seat_pointer_request_set_cursor_event *event=data;
  struct wlr_seat_client *focused_client=server->seat->pointer_state.focused_client;
  //can be sent by any client, so check to make sure this one actually has pointer focus first
  if(focused_client==event->seat_client){
    wlr_cursor_set_surface(server->cursor,event->surface,event->hotspot_x,event->hotspot_y);
  }
}

static void seat_pointer_focus_change(struct wl_listener *listener,void *data){
  struct sandwl_server *server = wl_container_of(listener,server,pointer_focus_change);
  //raised when the pointer focus is changed, including when the client is closed
  //set the cursor image to its default if target surface is NULL
  struct wlr_seat_pointer_focus_change_event *event=data;
  if(event->new_surface==NULL){
    wlr_cursor_set_xcursor(server->cursor,server->cursor_mgr,"default");
  }
}

static void seat_request_set_selection(struct wl_listener *listener,void *data){
  //raised by the seat when a client wants to set the selection, usually when the user copies something
  struct sandwl_server *server=wl_container_of(listener,server,request_set_selection);
  struct wlr_seat_request_set_selection_event *event=data;
  wlr_seat_set_selection(server->seat,event->source,event->serial);
}


int main(int argc, char *argv[]){
  wlr_log_init(WLR_DEBUG,NULL);
  char *startup_cmd=NULL;

  int c;
  while((c=getopt(argc,argv,"s:h"))!=-1){
    switch(c){
      case 's':
        startup_cmd=optarg;
        break;
      default:
        printf("Usage: %s [-s startup command]\n",argv[0]);
        return 0;
    }
  }
  if(optind<argc){
    printf("Usage: %s [-s startup command]\n",argv[0]);
    return 0;
  }

  struct sandwl_server server={0};
  server.wl_display=wl_display_create();
  assert(server.wl_display);
  server.wl_event_loop=wl_display_get_event_loop(server.wl_display);
  assert(server.wl_event_loop);
  server.backend=wlr_backend_autocreate(server.wl_event_loop,NULL);
  if(server.backend==NULL){
    wlr_log(WLR_ERROR,"failed to create wlr_backend");return 1;
  }
  server.renderer=wlr_renderer_autocreate(server.backend);
  if(server.renderer==NULL){
    wlr_log(WLR_ERROR,"failed to create wlr_renderer");return 1;
  }
  wlr_renderer_init_wl_display(server.renderer, server.wl_display);
  server.allocator=wlr_allocator_autocreate(server.backend,server.renderer);
  if(server.allocator==NULL){
    wlr_log(WLR_ERROR,"failed to create wlr_allocator");return 1;
  }

  //compositor is necessary for clients to allocate surfaces
  wlr_compositor_create(server.wl_display,5,server.renderer);
  //subcompositor allows to assign the role of subsurfaces to surfaces
  wlr_subcompositor_create(server.wl_display);
  //data device manager handles the clipboard
  wlr_data_device_manager_create(server.wl_display);

  //creates an output layout, saves the positions of outputs
  server.output_layout=wlr_output_layout_create(server.wl_display);

  //notify for new outputs
  wl_list_init(&server.outputs);
  server.new_output.notify=server_new_output;
  wl_signal_add(&server.backend->events.new_output,&server.new_output);


  server.scene=wlr_scene_create();
  server.scene_layout=wlr_scene_attach_output_layout(server.scene,server.output_layout);


  //Set up xdg-shell version 3, who manages applications windows
  wl_list_init(&server.toplevels);
  server.xdg_shell=wlr_xdg_shell_create(server.wl_display,3);
  server.new_xdg_toplevel.notify=server_new_xdg_toplevel;
  wl_signal_add(&server.xdg_shell->events.new_toplevel,&server.new_xdg_toplevel);
  server.new_xdg_popup.notify=server_new_xdg_popup;
  wl_signal_add(&server.xdg_shell->events.new_popup,&server.new_xdg_popup);

  server.cursor=wlr_cursor_create();
  wlr_cursor_attach_output_layout(server.cursor,server.output_layout);
  server.cursor_mgr=wlr_xcursor_manager_create(NULL,24);

  //cursor functionality
  server.cursor_mode=SANDWL_CURSOR_PASSTHROUGH;
  server.cursor_motion.notify=server_cursor_motion;
  wl_signal_add(&server.cursor->events.motion,&server.cursor_motion);
  server.cursor_motion_absolute.notify=server_cursor_motion_absolute;
  wl_signal_add(&server.cursor->events.motion_absolute,&server.cursor_motion_absolute);
  server.cursor_button.notify=server_cursor_button;
  wl_signal_add(&server.cursor->events.button,&server.cursor_button);
  server.cursor_axis.notify=server_cursor_axis;
  wl_signal_add(&server.cursor->events.axis,&server.cursor_axis);
  server.cursor_frame.notify=server_cursor_frame;
  wl_signal_add(&server.cursor->events.frame,&server.cursor_frame);

  //configures a "seat"(up to one keyboard, pointer, touch, and drawing tablet device)
  //and a listener for new devices
  wl_list_init(&server.keyboards);
  server.new_input.notify=server_new_input;
  wl_signal_add(&server.backend->events.new_input,&server.new_input);
  server.seat=wlr_seat_create(server.wl_display,"seat0");
  server.request_cursor.notify=seat_request_cursor;
  wl_signal_add(&server.seat->events.request_set_cursor,&server.request_cursor);
  server.pointer_focus_change.notify=seat_pointer_focus_change;
  wl_signal_add(&server.seat->pointer_state.events.focus_change,&server.pointer_focus_change);
  server.request_set_selection.notify=seat_request_set_selection;
  wl_signal_add(&server.seat->events.request_set_selection,&server.request_set_selection);


  //Add a Unix socket to the Wayland display
  const char *socket=wl_display_add_socket_auto(server.wl_display);
  if(!socket){
    wlr_backend_destroy(server.backend);
    return 1;
  }

  //starts the backend
  //this enumerates outputs and inputs
  if(!wlr_backend_start(server.backend)){
    wlr_log(WLR_ERROR,"Failed to start backend\n");
    wlr_backend_destroy(server.backend);
    wl_display_destroy(server.wl_display);
    return 1;
  }

  setenv("WAYLAND_DISPLAY",socket,true);
  if(startup_cmd){
    if(fork()==0){
      execl("/bin/sh","/bin/sh","-c",startup_cmd,(void*)NULL);
    }
  }

  wlr_log(WLR_INFO,"Running Wayland compositor on WAYLAND_DISPLAY=%s",socket);
  wl_display_run(server.wl_display);


  wl_display_destroy_clients(server.wl_display);

  wl_list_remove(&server.new_xdg_toplevel.link);
  wl_list_remove(&server.new_xdg_popup.link);

  wl_list_remove(&server.cursor_motion.link);
  wl_list_remove(&server.cursor_motion_absolute.link);
  wl_list_remove(&server.cursor_button.link);
  wl_list_remove(&server.cursor_axis.link);
  wl_list_remove(&server.cursor_frame.link);

  wl_list_remove(&server.new_input.link);
  wl_list_remove(&server.request_cursor.link);
  wl_list_remove(&server.pointer_focus_change.link);
  wl_list_remove(&server.request_set_selection.link);

  wl_list_remove(&server.new_output.link);

  wlr_scene_node_destroy(&server.scene->tree.node);
  wlr_xcursor_manager_destroy(server.cursor_mgr);
  wlr_cursor_destroy(server.cursor);
  wlr_allocator_destroy(server.allocator);
  wlr_renderer_destroy(server.renderer);
  wlr_backend_destroy(server.backend);
  wl_display_destroy(server.wl_display);
  return 0;
}
