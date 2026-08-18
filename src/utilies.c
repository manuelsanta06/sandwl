#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>

#include "types.h"
#include "utilies.h"


void focus_surface(struct sandwl_server *server,struct wlr_scene_tree *tree,struct wlr_surface *surface){
  if(!surface)return;
  struct wlr_seat *seat=server->seat;
  struct wlr_surface *prev_surface=seat->keyboard_state.focused_surface;
  if(prev_surface==surface)return;

  //Deactivate previous surface
  if(prev_surface){
    struct wlr_surface *prev_root=wlr_surface_get_root_surface(prev_surface);
    struct wlr_xdg_surface *prev_xdg=wlr_xdg_surface_try_from_wlr_surface(prev_root);
    if(prev_xdg&&prev_xdg->role==WLR_XDG_SURFACE_ROLE_TOPLEVEL){
      wlr_xdg_toplevel_set_activated(prev_xdg->toplevel,false);
    }
    struct wlr_xwayland_surface *prev_xway=wlr_xwayland_surface_try_from_wlr_surface(prev_root);
    if(prev_xway){
      wlr_xwayland_surface_activate(prev_xway,false);
    }
  }

  struct wlr_keyboard *keyboard=wlr_seat_get_keyboard(seat);

  if(tree)
    wlr_scene_node_raise_to_top(&tree->node);

  //Activate new surface
  struct wlr_surface *root_surface=wlr_surface_get_root_surface(surface);
  struct wlr_xdg_surface *xdg=wlr_xdg_surface_try_from_wlr_surface(root_surface);
  if(xdg&&xdg->role==WLR_XDG_SURFACE_ROLE_TOPLEVEL){
    wlr_xdg_toplevel_set_activated(xdg->toplevel,true);
    //Reordenar native list
    struct sandwl_toplevel *toplevel=tree->node.data;
    if(toplevel){
      wl_list_remove(&toplevel->link);
      wl_list_insert(&server->toplevels,&toplevel->link);
    }
  }
  struct wlr_xwayland_surface *xway=wlr_xwayland_surface_try_from_wlr_surface(root_surface);
  if(xway){
    wlr_xwayland_surface_activate(xway,true);
    //Reordenar x11 list
    struct sandwl_xwayland_surface *xway_surf=tree->node.data;
    if(xway_surf){
      wl_list_remove(&xway_surf->link);
      wl_list_insert(&server->toplevels,&xway_surf->link);
    }
  }

  if(keyboard)
    wlr_seat_keyboard_notify_enter(seat,surface,keyboard->keycodes,keyboard->num_keycodes,&keyboard->modifiers);
}

void reset_cursor_mode(struct sandwl_server *server){
  //Reset the cursor mode to passthrough
  server->cursor_mode=SANDWL_CURSOR_PASSTHROUGH;
  server->grabbed_tree=NULL;
}

//return the topmost node in the scene at a given layout coords
struct wlr_scene_tree *desktop_tree_at(
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

  //find the node corresponding to the root of this surface tree
  struct wlr_scene_tree *tree=node->parent;
  while(tree!=NULL&&tree->node.data==NULL)
    tree=tree->node.parent;
  if(tree==NULL)
    return NULL;

  struct wlr_surface *root_surface=wlr_surface_get_root_surface(*surface);
  struct wlr_xdg_surface *xdg_surface=wlr_xdg_surface_try_from_wlr_surface(root_surface);
  struct wlr_xwayland_surface *xwayland_surface=wlr_xwayland_surface_try_from_wlr_surface(root_surface);
  
  if(!xdg_surface&&!xwayland_surface)
    return NULL;
  
  if(xdg_surface && xdg_surface->role!=WLR_XDG_SURFACE_ROLE_TOPLEVEL)
    return NULL;
  
  return tree;
}
