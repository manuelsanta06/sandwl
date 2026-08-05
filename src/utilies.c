#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>

#include "types.h"
#include "utilies.h"


void focus_toplevel(struct sandwl_toplevel *toplevel){
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

void reset_cursor_mode(struct sandwl_server *server){
  //Reset the cursor mode to passthrough
  server->cursor_mode=SANDWL_CURSOR_PASSTHROUGH;
  server->grabbed_toplevel=NULL;
}

//return the topmost node in the scene at a given layout coords
struct sandwl_toplevel *desktop_toplevel_at(
  struct sandwl_server *server,double lx,double ly,
  struct wlr_surface **surface,double*sx,double*sy){
  struct wlr_scene_node *node=wlr_scene_node_at(&server->scene->tree.node,lx,ly,sx,sy);
  if(node==NULL||node->type!=WLR_SCENE_NODE_BUFFER)
    return NULL;

  struct wlr_scene_buffer *scene_buffer=wlr_scene_buffer_from_node(node);
  struct wlr_scene_surface *scene_surface=wlr_scene_surface_try_from_buffer(scene_buffer);
  if(!scene_surface)
    return NULL;

  *surface=scene_surface->surface;

  //find the node corresponding to the sandwl_toplevel at the root of this surface tree
  struct wlr_scene_tree *tree=node->parent;
  while(tree!=NULL&&tree->node.data==NULL)
    tree=tree->node.parent;
  if(tree==NULL)
    return NULL;

  struct wlr_surface *root_surface=wlr_surface_get_root_surface(*surface);
  struct wlr_xdg_surface *xdg_surface=wlr_xdg_surface_try_from_wlr_surface(root_surface);
  
  if(!xdg_surface||xdg_surface->role!=WLR_XDG_SURFACE_ROLE_TOPLEVEL)
    return NULL;
  
  return tree->node.data;
}
