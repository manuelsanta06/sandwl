#pragma once

#include <stdint.h>
#include <wayland-server-core.h>
#include <wlr/util/box.h>


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

  struct wlr_layer_shell_v1       *layer_shell;
  struct wl_listener              new_layer_surface;
};

struct sandwl_output{
  struct wl_list        link;
  struct sandwl_server  *server;
  struct wlr_output     *wlr_output;
  struct wlr_box        usable_area;
  struct wl_list        layers[4];
  struct wl_listener    frame;
  struct wl_listener    request_state;
  struct wl_listener    destroy;
};

struct sandwl_layer_surface{
  struct wl_list                    link;
  struct sandwl_server              *server;
  struct wlr_layer_surface_v1       *layer_surface;
  struct wlr_scene_layer_surface_v1 *scene_layer_surface;
  struct wlr_scene_tree             *scene_tree;
  struct wl_listener                map;
  struct wl_listener                unmap;
  struct wl_listener                destroy;
  struct wl_listener                surface_commit;
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
