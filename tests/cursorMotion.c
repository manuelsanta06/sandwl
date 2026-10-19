#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <wlr/backend/headless.h>
#include <wlr/interfaces/wlr_pointer.h>
#include <wlr/render/allocator.h>
#include <wlr/render/pixman.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_output_layout.h>

#include "types.h"
#include "serverCursor.h"

static void check_motion(struct sandwl_server *server,double yaw,double pitch){
  if(fabs(server->camera_mode.yaw_delta-yaw)>=0.000001||
      fabs(server->camera_mode.pitch_delta-pitch)>=0.000001)
    fprintf(stderr,"Expected rotation (%g,%g), got (%g,%g)\n",
      yaw,pitch,server->camera_mode.yaw_delta,server->camera_mode.pitch_delta);
  assert(fabs(server->camera_mode.yaw_delta-yaw)<0.000001);
  assert(fabs(server->camera_mode.pitch_delta-pitch)<0.000001);
}

static void absolute(struct sandwl_server *server,struct wlr_pointer *pointer,double x,double y){
  struct wlr_pointer_motion_absolute_event event={
    .pointer=pointer,
    .x=x,
    .y=y,
  };
  server_cursor_motion_absolute(&server->cursor_motion_absolute,&event);
}

static void relative(struct sandwl_server *server,struct wlr_pointer *pointer,double dx,double dy){
  struct wlr_pointer_motion_event event={
    .pointer=pointer,
    .delta_x=dx*2,
    .delta_y=dy*2,
    .unaccel_dx=dx,
    .unaccel_dy=dy,
  };
  server_cursor_motion(&server->cursor_motion,&event);
}

int main(void){
  struct sandwl_server server={0};
  wl_list_init(&server.keyboards);
  wl_list_init(&server.pointers);
  wl_list_init(&server.camera_mode.bindings);
  server.wl_display=wl_display_create();
  assert(server.wl_display);
  server.backend=wlr_headless_backend_create(wl_display_get_event_loop(server.wl_display));
  assert(server.backend);
  struct wlr_output *output=wlr_headless_add_output(server.backend,1000,800);
  assert(output);
  server.renderer=wlr_pixman_renderer_create();
  assert(server.renderer);
  server.allocator=wlr_allocator_autocreate(server.backend,server.renderer);
  assert(server.allocator);
  assert(wlr_output_init_render(output,server.allocator,server.renderer));
  assert(wlr_backend_start(server.backend));
  server.output_layout=wlr_output_layout_create(server.wl_display);
  assert(server.output_layout);
  assert(wlr_output_layout_add(server.output_layout,output,0,0));
  server.cursor=wlr_cursor_create();
  assert(server.cursor);
  wlr_cursor_attach_output_layout(server.cursor,server.output_layout);
  wlr_cursor_warp_closest(server.cursor,NULL,500,400);
  double cursor_x=server.cursor->x,cursor_y=server.cursor->y;

  const struct wlr_pointer_impl impl={.name="test"};
  struct wlr_pointer first,second,hybrid,mouse;
  wlr_pointer_init(&first,&impl,"first");
  wlr_pointer_init(&second,&impl,"second");
  wlr_pointer_init(&hybrid,&impl,"hybrid");
  wlr_pointer_init(&mouse,&impl,"mouse");
  server_new_pointer(&server,&first.base);
  server_new_pointer(&server,&second.base);
  server_new_pointer(&server,&hybrid.base);
  server_new_pointer(&server,&mouse.base);

  camera_mode_add_motion(&server,100,100);
  check_motion(&server,0,0);
  sandwl_camera_mode_set_enabled(&server,NULL,true);
  absolute(&server,&first,0.2,0.3);
  check_motion(&server,0,0);
  absolute(&server,&first,0.3,0.4);
  check_motion(&server,10,-8);
  absolute(&server,&second,0.8,0.9);
  check_motion(&server,10,-8);
  absolute(&server,&second,0.7,0.8);
  check_motion(&server,0,0);

  sandwl_camera_mode_clear_motion(&server);
  absolute(&server,&first,0.4,0.4);
  check_motion(&server,10,0);
  sandwl_camera_mode_set_enabled(&server,NULL,false);
  check_motion(&server,0,0);
  sandwl_camera_mode_set_enabled(&server,NULL,true);
  absolute(&server,&first,0.9,0.9);
  check_motion(&server,0,0);
  absolute(&server,&first,NAN,0.9);
  absolute(&server,&first,0.1,0.1);
  check_motion(&server,0,0);

  relative(&server,&mouse,3,-4);
  relative(&server,&mouse,-1,2);
  check_motion(&server,0.2,0.2);
  absolute(&server,&hybrid,0.1,0.1);
  absolute(&server,&hybrid,0.2,0.2);
  check_motion(&server,10.2,-7.8);
  relative(&server,&hybrid,100,80);
  check_motion(&server,10.2,-7.8);
  absolute(&server,&hybrid,0.3,0.3);
  check_motion(&server,10.2,-7.8);

  sandwl_camera_mode_clear_motion(&server);
  relative(&server,&hybrid,1,1);
  check_motion(&server,0.1,-0.1);
  relative(&server,&hybrid,NAN,1);
  check_motion(&server,0.1,-0.1);
  assert(server.cursor->x==cursor_x&&server.cursor->y==cursor_y);

  wlr_pointer_finish(&first);
  wlr_pointer_finish(&second);
  wlr_pointer_finish(&hybrid);
  wlr_pointer_finish(&mouse);
  assert(wl_list_empty(&server.pointers));
  sandwl_camera_mode_destroy(&server,NULL);
  check_motion(&server,0,0);
  wlr_cursor_destroy(server.cursor);
  wlr_backend_destroy(server.backend);
  wlr_allocator_destroy(server.allocator);
  wlr_renderer_destroy(server.renderer);
  wl_display_destroy(server.wl_display);
  puts("Cursor motion tests passed");
  return 0;
}
