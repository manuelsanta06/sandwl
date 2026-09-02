#include <getopt.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <wlr/backend.h>

#include <wlr/render/wlr_renderer.h>
#include <wlr/render/allocator.h>

#include <wlr/types/wlr_pointer_constraints_v1.h>
#include <wlr/types/wlr_primary_selection_v1.h>
#include <wlr/types/wlr_pointer_gestures_v1.h>
#include <wlr/types/wlr_relative_pointer_v1.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_data_control_v1.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_viewporter.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>

#include <wlr/util/log.h>

#include "serverKeyboard.h"
#include "serverCursor.h"
#include "xdgToplevel.h"
#include "decoration.h"
#include "layerShell.h"
#include "xwayland.h"
#include "outputs.h"
#include "types.h"
#include "seats.h"
#include "config/luaConfig.h"


void server_new_input(struct wl_listener *listener,void *data){
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

void printHelp(char* programName){
  printf("Usage: %s\n",programName);
  printf("\t-s [command]  runs [command] after starting the compositor\n");
  printf("\t-v            sets verbosity to DEBUG level\n");
}

static void server_remove_listeners(struct sandwl_server *server){
  struct wl_listener *listeners[]={
    &server->new_xdg_toplevel,
    &server->new_xdg_popup,
    &server->new_layer_surface,
    &server->new_xdg_decoration,
    &server->cursor_motion,
    &server->cursor_motion_absolute,
    &server->cursor_button,
    &server->cursor_axis,
    &server->cursor_frame,
    &server->new_pointer_constraint,
    &server->new_input,
    &server->request_cursor,
    &server->pointer_focus_change,
    &server->request_set_selection,
    &server->request_set_primary_selection,
    &server->request_start_drag,
    &server->start_drag,
    &server->new_output,
  };

  for(size_t i=0;i<sizeof(listeners)/sizeof(listeners[0]);i++){
    if(listeners[i] && listeners[i]->link.next){
      wl_list_remove(&listeners[i]->link);
    }
  }

  if(server->xwayland_ready.link.next){
    wl_list_remove(&server->xwayland_ready.link);
  }
  if(server->new_xwayland_surface.link.next){
    wl_list_remove(&server->new_xwayland_surface.link);
  }
}

int main(int argc, char *argv[]){
  wlr_log_init(WLR_ERROR,NULL);
  char *startup_cmd=NULL;

  int c;
  while((c=getopt(argc,argv,"vs:h"))!=-1){
    switch(c){
      case 'v':
        wlr_log_init(WLR_DEBUG,NULL);
        break;
      case 's':
        startup_cmd=optarg;
        break;
      default:
        printHelp(argv[0]);
        return 0;
    }
  }
  if(optind<argc){
    printHelp(argv[0]);
    return 0;
  }

  struct sandwl_server server={0};
  server.wl_display=wl_display_create();
  assert(server.wl_display);
  server.wl_event_loop=wl_display_get_event_loop(server.wl_display);
  assert(server.wl_event_loop);
  server.backend=wlr_backend_autocreate(server.wl_event_loop,&server.session);
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
  server.compositor=wlr_compositor_create(server.wl_display,5,server.renderer);
  //subcompositor allows to assign the role of subsurfaces to surfaces
  wlr_subcompositor_create(server.wl_display);

  //Initialize XWayland
  //The boolean true enables lazy loading (starts XWayland only when an X11 client connects)
  server.xwayland=wlr_xwayland_create(server.wl_display,server.compositor,true);
  if(server.xwayland){
    setenv("DISPLAY",server.xwayland->display_name,true);
    server.xwayland_ready.notify=server_xwayland_ready;
    wl_signal_add(&server.xwayland->events.ready,&server.xwayland_ready);
    server.new_xwayland_surface.notify=server_new_xwayland_surface;
    wl_signal_add(&server.xwayland->events.new_surface,&server.new_xwayland_surface);
  }else{
    wlr_log(WLR_ERROR,"XWayland unavailable; continuing without XWayland");
  }

  //data device manager handles the clipboard
  wlr_data_device_manager_create(server.wl_display);
  wlr_data_control_manager_v1_create(server.wl_display);
  wlr_primary_selection_v1_device_manager_create(server.wl_display);

  //creates an output layout, saves the positions of outputs
  server.output_layout=wlr_output_layout_create(server.wl_display);

  wlr_xdg_output_manager_v1_create(server.wl_display,server.output_layout);
  wlr_viewporter_create(server.wl_display);

  //notify for new outputs
  wl_list_init(&server.outputs);
  server.new_output.notify=server_new_output;
  wl_signal_add(&server.backend->events.new_output,&server.new_output);


  server.scene=wlr_scene_create();
  server.scene_layout=wlr_scene_attach_output_layout(server.scene,server.output_layout);

  server.scene_background=wlr_scene_tree_create(&server.scene->tree);
  server.scene_bottom=wlr_scene_tree_create(&server.scene->tree);
  server.scene_toplevel=wlr_scene_tree_create(&server.scene->tree);
  server.scene_top=wlr_scene_tree_create(&server.scene->tree);
  server.scene_overlay=wlr_scene_tree_create(&server.scene->tree);


  //Set up xdg-shell version 3, who manages applications windows
  wl_list_init(&server.toplevels);
  server.xdg_shell=wlr_xdg_shell_create(server.wl_display,3);
  server.new_xdg_toplevel.notify=server_new_xdg_toplevel;
  wl_signal_add(&server.xdg_shell->events.new_toplevel,&server.new_xdg_toplevel);
  server.new_xdg_popup.notify=server_new_xdg_popup;
  wl_signal_add(&server.xdg_shell->events.new_popup,&server.new_xdg_popup);

  //layer-shell
  server.layer_shell=wlr_layer_shell_v1_create(server.wl_display,4);
  server.new_layer_surface.notify=server_new_layer_surface;
  wl_signal_add(&server.layer_shell->events.new_surface,&server.new_layer_surface);

  server.xdg_decoration_manager=wlr_xdg_decoration_manager_v1_create(server.wl_display);
  server.new_xdg_decoration.notify=server_new_xdg_decoration;
  wl_signal_add(&server.xdg_decoration_manager->events.new_toplevel_decoration,&server.new_xdg_decoration);

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

  server.pointer_gestures=wlr_pointer_gestures_v1_create(server.wl_display);

  server.relative_pointer_manager=wlr_relative_pointer_manager_v1_create(server.wl_display);

  server.pointer_constraints=wlr_pointer_constraints_v1_create(server.wl_display);
  server.new_pointer_constraint.notify=handle_new_pointer_constraint;
  wl_signal_add(&server.pointer_constraints->events.new_constraint,&server.new_pointer_constraint);


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
  server.request_set_primary_selection.notify=seat_request_set_primary_selection;
  wl_signal_add(&server.seat->events.request_set_primary_selection,&server.request_set_primary_selection);

  server.request_start_drag.notify=seat_request_start_drag;
  wl_signal_add(&server.seat->events.request_start_drag,&server.request_start_drag);
  server.start_drag.notify=seat_start_drag;
  wl_signal_add(&server.seat->events.start_drag,&server.start_drag);

  //add a unix socket to the wayland display
  const char *socket=wl_display_add_socket_auto(server.wl_display);
  if(!socket){
    server_remove_listeners(&server);
    wlr_backend_destroy(server.backend);
    wl_display_destroy(server.wl_display);
    return 1;
  }
  setenv("WAYLAND_DISPLAY",socket,true);

  //run lua config
  server.lua=sandwl_lua_create();
  if(!server.lua){
    server_remove_listeners(&server);
    wlr_backend_destroy(server.backend);
    wl_display_destroy(server.wl_display);
    return 1;
  }

  enum sandwl_lua_config_result config_result=sandwl_lua_load_config(server.lua,NULL);
  if(config_result==SANDWL_LUA_CONFIG_FAILED){
    sandwl_lua_destroy(server.lua);
    server_remove_listeners(&server);
    wlr_backend_destroy(server.backend);
    wl_display_destroy(server.wl_display);
    return 1;
  }

  //starts the backend
  //this enumerates outputs and inputs
  if(!wlr_backend_start(server.backend)){
    wlr_log(WLR_ERROR,"Failed to start backend\n");
    sandwl_lua_destroy(server.lua);
    server_remove_listeners(&server);
    wlr_backend_destroy(server.backend);
    wl_display_destroy(server.wl_display);
    return 1;
  }

  if(startup_cmd){
    if(fork()==0){
      execl("/bin/sh","/bin/sh","-c",startup_cmd,(void*)NULL);
    }
  }

  wlr_log(WLR_INFO,"Running Wayland compositor on WAYLAND_DISPLAY=%s",socket);
  wl_display_run(server.wl_display);


  wl_display_destroy_clients(server.wl_display);
  server_remove_listeners(&server);

  wlr_scene_node_destroy(&server.scene->tree.node);
  sandwl_lua_destroy(server.lua);
  wlr_xcursor_manager_destroy(server.cursor_mgr);
  wlr_cursor_destroy(server.cursor);
  wlr_allocator_destroy(server.allocator);
  wlr_renderer_destroy(server.renderer);
  wlr_backend_destroy(server.backend);
  wl_display_destroy(server.wl_display);
  return 0;
}
