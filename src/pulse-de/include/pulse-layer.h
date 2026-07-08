/* SPDX-License-Identifier: MIT */
/**
 * pulse-layer.h — layer-shell surface management for PulseDE.
 *
 * The layer-shell protocol (wlr-layer-shell-unstable-v1) is the mechanism
 * used by all Pulse desktop shell components: the panel, future launchers,
 * notification overlays, and lock-screen surfaces. It allows clients to
 * request placement at specific Z-layers (background/bottom/top/overlay)
 * and anchor to screen edges with a configurable exclusive zone.
 *
 * This module:
 *  1. Creates and advertises wlr_layer_shell_v1 on the compositor's display.
 *  2. Handles new layer surfaces — places them in the correct scene tree.
 *  3. Configures layer surfaces with output dimensions on map.
 *  4. Manages exclusive zones so panels can reserve space from window tiling.
 *  5. Enforces the correct Z-ordering of the four layer scene trees relative
 *     to normal XDG windows.
 *
 * Scene Z-order (bottom to top):
 *   layer_tree_background  — desktop wallpaper / background surfaces
 *   layer_tree_bottom      — panels sitting behind windows (rare)
 *   [XDG toplevels]        — normal application windows
 *   layer_tree_top         — panels sitting above windows (Pulse Panel uses this)
 *   layer_tree_overlay     — lock screen, screenshot UI, always-on-top overlays
 *
 * The Pulse Panel uses LAYER_TOP so it renders above all application windows.
 */

#pragma once

#include <wayland-server-core.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_scene.h>

struct pulse_server;
struct pulse_output;

/**
 * pulse_layer_surface — compositor-side state for one layer shell surface.
 */
struct pulse_layer_surface {
    struct wl_list                   link;    /* server->layer_surfaces */
    struct pulse_server             *server;
    struct wlr_layer_surface_v1     *wlr_layer_surface;
    struct wlr_scene_layer_surface_v1 *scene_layer_surface;
    struct wlr_scene_tree            *scene_tree;

    struct wl_listener  map;
    struct wl_listener  unmap;
    struct wl_listener  commit;
    struct wl_listener  destroy;
    struct wl_listener  new_popup;
};

/**
 * layer_init() — create the layer shell manager and layer scene trees.
 *
 * Must be called after the scene graph and output layout are initialised.
 * Creates four wlr_scene_tree children of server->scene->tree in correct
 * Z order, and registers the new_layer_surface listener.
 *
 * Returns false on allocation failure.
 */
bool layer_init(struct pulse_server *server);

/**
 * layer_arrange() — re-arrange all layer surfaces on an output after
 * a geometry change (output resize, new output, surface map/unmap).
 *
 * Recomputes exclusive zones and sends configure events with updated
 * geometry to all layer surfaces on the given output.
 */
void layer_arrange(struct pulse_server *server, struct pulse_output *output);
