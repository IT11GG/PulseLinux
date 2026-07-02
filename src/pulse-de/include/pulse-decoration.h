/* SPDX-License-Identifier: MIT */
/**
 * pulse-decoration.h — server-side window decoration management.
 *
 * PulseDE uses server-side decorations (SSD) exclusively. This is a
 * deliberate design choice matching Pulse HIG §1.1: window controls must
 * always be in the same position in every app, which is only guaranteed
 * when the compositor draws them — not the client.
 *
 * This module:
 *  1. Creates a wlr_xdg_decoration_manager_v1, advertising SSD support.
 *  2. Handles new_toplevel_decoration events by requesting SERVER mode.
 *  3. Draws per-window scene-graph decorations (border rects) that track
 *     each toplevel's geometry and focus state.
 *
 * Milestone 2 scope: borders only (a 1px top accent line for focus,
 * a dim border on all four sides). The full Pulse titlebar (with window
 * controls, title text, and the Pulse Beat hairline) is Milestone 3.
 */

#pragma once

#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>

struct pulse_server;

/**
 * pulse_window_border — scene-graph border overlaid on a toplevel window.
 *
 * Composed of five wlr_scene_rect nodes forming an outer border frame.
 * The top border doubles as the focus indicator: Signal red (#FF2438) when
 * focused, Surface-1 (#0E0C0D) when unfocused, per the design spec.
 */
struct pulse_window_border {
    struct wlr_scene_rect *top;
    struct wlr_scene_rect *bottom;
    struct wlr_scene_rect *left;
    struct wlr_scene_rect *right;
};

/**
 * decoration_init() - create the xdg-decoration manager and register
 * the new_toplevel_decoration listener on the server.
 *
 * Must be called after server->xdg_shell is initialised.
 * Returns false on allocation failure.
 */
bool decoration_init(struct pulse_server *server);

/**
 * decoration_create_border() - allocate and position border rects for a
 * toplevel window. Called from xdg-shell.c when a toplevel is mapped.
 *
 * The returned pulse_window_border is owned by the caller (pulse_toplevel)
 * and must be freed with decoration_destroy_border() on unmap/destroy.
 */
struct pulse_window_border *decoration_create_border(
    struct pulse_server *server,
    struct wlr_scene_tree *toplevel_tree,
    int width, int height, bool focused);

/**
 * decoration_update_border() - reposition and recolour border rects after
 * a geometry or focus change.
 */
void decoration_update_border(struct pulse_window_border *border,
                               int width, int height, bool focused);

/**
 * decoration_destroy_border() - destroy border scene rects and free.
 */
void decoration_destroy_border(struct pulse_window_border *border);
