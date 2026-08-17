#include <wlr/types/wlr_scene.h>

void server_xwayland_ready(struct wl_listener *listener,void *data);

void xwayland_surface_map(struct wl_listener *listener,void *data);

void xwayland_surface_unmap(struct wl_listener *listener,void *data);

void xwayland_surface_destroy(struct wl_listener *listener,void *data);

void xwayland_surface_request_configure(struct wl_listener *listener,void *data);

void xwayland_surface_request_move(struct wl_listener *listener,void *data);

void xwayland_surface_request_resize(struct wl_listener *listener,void *data);

void server_new_xwayland_surface(struct wl_listener *listener,void *data);

