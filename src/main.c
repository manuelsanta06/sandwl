#include <getopt.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/types/wlr_viewporter.h>

#include "types.h"
#include "seats.h"
#include "outputs.h"
#include "xdgToplevel.h"
#include "serverCursor.h"
#include "serverKeyboard.h"
#include "layerShell.h"


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

  wlr_xdg_output_manager_v1_create(server.wl_display,server.output_layout);
  wlr_viewporter_create(server.wl_display);

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

  //layer-shell
  server.layer_shell=wlr_layer_shell_v1_create(server.wl_display,4);
  server.new_layer_surface.notify=server_new_layer_surface;
  wl_signal_add(&server.layer_shell->events.new_surface,&server.new_layer_surface);

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
