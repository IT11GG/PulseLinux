/* SPDX-License-Identifier: MIT */
/**
 * pulse-server.h — PulseDE compositor central state
 *
 * The pulse_server struct is the single root object that owns every piece
 * of compositor state. There is exactly one instance per running session,
 * allocated in main() and passed by pointer to every subsystem.
 *
 * This mirrors the wlroots convention (struct tinywl_server, etc.) and
 * is the right shape for a compositor: one clean root rather than scattered
 * globals, making the lifetime of every resource explicit.
 */

#pragma once

#include <stdbool.h>
#include "pulse-grab.h"
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/edges.h>
#include <wlr/util/log.h>

/* Forward declarations for subsystem types defined in their own headers */
struct pulse_output;
struct pulse_toplevel;
struct pulse_keyboard;

/**
 * pulse_server - root compositor state
 *
 * Owns every wlroots object for the lifetime of the session. Members are
 * grouped by subsystem and ordered by initialisation order in server_init().
 */
struct pulse_server {
    /* ---- Core Wayland / wlroots objects -------------------------------- */
    struct wl_display        *wl_display;
    struct wl_event_loop     *event_loop;
    struct wlr_backend       *backend;
    struct wlr_renderer      *renderer;
    struct wlr_allocator     *allocator;

    /* ---- Scene graph (wlr_scene — hardware-accelerated damage tracking) */
    struct wlr_scene         *scene;
    struct wlr_scene_output_layout *scene_layout;

    /* ---- Layer scene trees (one per layer-shell layer, in Z order) ----- */
    /* background < bottom < [xdg windows] < top < overlay                  */
    struct wlr_scene_tree     *layer_tree_background;
    struct wlr_scene_tree     *layer_tree_bottom;
    struct wlr_scene_tree     *layer_tree_top;
    struct wlr_scene_tree     *layer_tree_overlay;

    /* ---- Protocols ------------------------------------------------------ */
    struct wlr_compositor    *compositor;
    struct wlr_subcompositor *subcompositor;
    struct wlr_data_device_manager *data_device_mgr;
    struct wlr_xdg_shell     *xdg_shell;
    struct wlr_xdg_decoration_manager_v1 *decoration_mgr;
    struct wlr_layer_shell_v1 *layer_shell;

    /* ---- Output management --------------------------------------------- */
    struct wlr_output_layout *output_layout;
    struct wl_list            outputs;   /* list of struct pulse_output */

    /* ---- Input / seat -------------------------------------------------- */
    struct wlr_seat          *seat;
    struct wlr_cursor        *cursor;
    struct wlr_xcursor_manager *xcursor_mgr;
    struct wl_list            keyboards; /* list of struct pulse_keyboard */

    /* ---- Window management state --------------------------------------- */
    struct wl_list            toplevels;      /* list of struct pulse_toplevel */
    struct wl_list            layer_surfaces;  /* list of struct pulse_layer_surface */
    struct pulse_toplevel    *focused_toplevel;

    /* ---- Interactive grab state (move / resize) ------------------------ */
    struct pulse_grab         grab;

    /* ---- Wayland listeners (wl_listener must live as long as the object
     *       it listens on — storing them in the server struct is idiomatic
     *       in wlroots compositors for session-lifetime listeners) -------- */
    struct wl_listener        new_output;
    struct wl_listener        new_xdg_surface;
    struct wl_listener        new_decoration;
    struct wl_listener        new_layer_surface;
    struct wl_listener        cursor_motion;
    struct wl_listener        cursor_motion_absolute;
    struct wl_listener        cursor_button;
    struct wl_listener        cursor_axis;
    struct wl_listener        cursor_frame;
    struct wl_listener        new_input;
    struct wl_listener        request_set_cursor;
    struct wl_listener        request_set_selection;
};

/* ---- Lifecycle ---------------------------------------------------------- */

/**
 * server_init() - allocate and initialise all compositor subsystems.
 *
 * On failure, logs the error with WLR_ERROR and returns false. The caller
 * (main.c) should exit cleanly without calling server_finish().
 */
bool server_init(struct pulse_server *server);

/**
 * server_finish() - destroy all compositor resources in reverse-init order.
 *
 * Safe to call after a partial server_init() failure only for the subsystems
 * that were successfully initialised — see server.c for the teardown guard
 * pattern used to handle this safely.
 */
void server_finish(struct pulse_server *server);

/**
 * server_run() - start the Wayland event loop.
 *
 * Blocks until the compositor should exit (SIGTERM, SIGINT, or a fatal
 * error from the backend). Returns false if the event loop encountered a
 * fatal error.
 */
bool server_run(struct pulse_server *server);
