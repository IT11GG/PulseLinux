/* SPDX-License-Identifier: MIT */
/**
 * pulse-xdg-shell.h — XDG shell window management state.
 *
 * pulse_toplevel wraps a wlr_xdg_toplevel (an application window) with
 * PulseDE-specific state: position in the scene graph, focus state,
 * and the Pulse Window Manager data that will be expanded in Milestone 3.
 */

#pragma once

#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_scene.h>

struct pulse_server;

/**
 * pulse_toplevel - an application window managed by PulseDE
 */
struct pulse_toplevel {
    struct wl_list             link;       /* server->toplevels */
    struct pulse_server       *server;
    struct wlr_xdg_toplevel   *xdg_toplevel;
    struct wlr_scene_tree     *scene_tree; /* scene node for this window */

    /* Listeners scoped to this toplevel's lifetime */
    struct wl_listener         map;
    struct wl_listener         unmap;
    struct wl_listener         commit;
    struct wl_listener         destroy;
    struct wl_listener         request_move;
    struct wl_listener         request_resize;
    struct wl_listener         request_maximize;
    struct wl_listener         request_fullscreen;
};

/**
 * pulse_popup - an XDG popup (context menu, tooltip, etc.)
 */
struct pulse_popup {
    struct wlr_xdg_popup   *xdg_popup;
    struct wlr_scene_tree  *scene_tree;

    struct wl_listener      commit;
    struct wl_listener      destroy;
};

/* Handler registered with server->xdg_shell.events.new_toplevel */
void xdg_shell_handle_new_toplevel(struct wl_listener *listener, void *data);

/* Handler registered with server->xdg_shell.events.new_popup */
void xdg_shell_handle_new_popup(struct wl_listener *listener, void *data);

/**
 * xdg_shell_focus_toplevel() - give keyboard focus to a toplevel.
 *
 * Implements the PulseDE focus invariant: exactly one toplevel holds
 * keyboard focus at any time. Updates wlr_seat, raises the window to the
 * top of the scene graph, and emits the activation signal (which the shell
 * layer uses to start the Pulse Beat Ambient animation on the titlebar edge).
 */
void xdg_shell_focus_toplevel(struct pulse_server *server,
                               struct pulse_toplevel *toplevel);

/**
 * xdg_shell_toplevel_at() - find the topmost toplevel under (lx, ly).
 *
 * Returns NULL if no toplevel is under the point. Sets *sx and *sy to the
 * surface-local coordinates of the hit, suitable for passing to
 * wlr_seat_pointer_notify_enter().
 */
struct pulse_toplevel *xdg_shell_toplevel_at(struct pulse_server *server,
                                              double lx, double ly,
                                              struct wlr_surface **surface,
                                              double *sx, double *sy);
