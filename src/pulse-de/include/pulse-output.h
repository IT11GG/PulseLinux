/* SPDX-License-Identifier: MIT */
/**
 * pulse-output.h — per-output (display) state for the PulseDE compositor.
 *
 * Each physical or virtual output (monitor) is represented by one
 * pulse_output. The struct is allocated when wlroots signals a new_output
 * event and freed when the output is destroyed.
 */

#pragma once

#include <wayland-server-core.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>

struct pulse_server;

/**
 * pulse_output - per-display compositor state
 */
struct pulse_output {
    struct wl_list       link;    /* server->outputs */
    struct pulse_server *server;
    struct wlr_output   *wlr_output;

    /* Background colour rect (--pulse-void #050505).
     * Stored so output_handle_request_state() can resize it when the
     * output resolution changes. */
    struct wlr_scene_rect *bg_rect;

    /* Listeners scoped to this output's lifetime */
    struct wl_listener   frame;
    struct wl_listener   request_state;
    struct wl_listener   destroy;
};

/**
 * output_handle_new() - called when wlroots signals a new output device.
 *
 * Allocates a pulse_output, configures the output with sensible defaults
 * (preferred mode, adaptive sync if available), and adds it to the scene
 * layout.
 */
void output_handle_new(struct wl_listener *listener, void *data);
