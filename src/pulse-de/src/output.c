/* SPDX-License-Identifier: MIT */
/**
 * output.c — per-output (display) management for PulseDE.
 *
 * Handles:
 *  - New output detection and configuration
 *  - Frame rendering (clearing to --pulse-void #050505)
 *  - Output state changes (resolution, refresh, adaptive sync)
 *  - Output destruction and resource cleanup
 *
 * Rendering is handled entirely by the wlr_scene / wlr_scene_output API,
 * which manages damage tracking, hardware planes, and renderer dispatch.
 * output.c's frame handler just drives the commit cycle; it does not call
 * into OpenGL or Pixman directly.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <wlr/backend.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "pulse-server.h"
#include "pulse-output.h"

/* ---------------------------------------------------------------------------
 * --pulse-void: #050505 as normalised float RGBA (alpha = 1.0)
 * Used as the compositor background colour on every frame, matching the
 * design token defined in src/tokens/tokens.css.
 * ---------------------------------------------------------------------------
 * Not pure black (0.0) because:
 *  a) Pulse design spec deliberately avoids literal #000000 to maintain
 *     perceived depth on OLED panels without burn-in risk from extreme contrast.
 *  b) Matches the warm-tinted black token that every visual layer inherits.
 */
#define PULSE_VOID_R (0x05 / 255.0f)
#define PULSE_VOID_G (0x05 / 255.0f)
#define PULSE_VOID_B (0x05 / 255.0f)
#define PULSE_VOID_A (1.0f)

/* ---------------------------------------------------------------------------
 * Frame handler — called once per vsync by wlroots
 * ------------------------------------------------------------------------- */
static void output_handle_frame(struct wl_listener *listener, void *data)
{
    struct pulse_output *output = wl_container_of(listener, output, frame);
    struct wlr_scene_output *scene_output =
        wlr_scene_get_scene_output(output->server->scene, output->wlr_output);

    if (!scene_output) {
        return;
    }

    /* wlr_scene_output_commit() composites the scene graph into a buffer,
     * applies damage optimisation, and sends the buffer to the output.
     * Passing NULL for the state parameter uses the output's current mode. */
    if (!wlr_scene_output_commit(scene_output, NULL)) {
        wlr_log(WLR_ERROR, "Failed to commit scene output for %s",
                output->wlr_output->name);
        return;
    }

    /* Send frame timing information to all surfaces that requested it
     * via the presentation-time protocol. */
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(scene_output, &now);
}

/* ---------------------------------------------------------------------------
 * Output state-change handler
 * Fires when the output's mode, transform, or scale changes.
 * ------------------------------------------------------------------------- */
static void output_handle_request_state(struct wl_listener *listener,
                                         void *data)
{
    struct pulse_output *output =
        wl_container_of(listener, output, request_state);
    const struct wlr_output_event_request_state *event = data;

    /* Commit the requested state immediately. For a production compositor
     * this would validate the requested mode against the output's supported
     * modes before committing. Milestone 1 accepts all requests. */
    wlr_output_commit_state(output->wlr_output, event->state);
}

/* ---------------------------------------------------------------------------
 * Output destroy handler
 * ------------------------------------------------------------------------- */
static void output_handle_destroy(struct wl_listener *listener, void *data)
{
    struct pulse_output *output = wl_container_of(listener, output, destroy);

    wlr_log(WLR_INFO, "Output destroyed: %s", output->wlr_output->name);

    wl_list_remove(&output->frame.link);
    wl_list_remove(&output->request_state.link);
    wl_list_remove(&output->destroy.link);
    wl_list_remove(&output->link);

    free(output);
}

/* ---------------------------------------------------------------------------
 * New output handler — called when backend signals a new display
 * ------------------------------------------------------------------------- */
void output_handle_new(struct wl_listener *listener, void *data)
{
    struct pulse_server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;

    /* Attach renderer — required before the output can render frames */
    wlr_output_init_render(wlr_output, server->allocator, server->renderer);

    /* Configure the output with its preferred mode (highest resolution at
     * the highest refresh rate the display reports as preferred).
     * wlr_output_state provides the transactional API introduced in
     * wlroots 0.17; we build the state, then commit it. */
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);

    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    if (mode != NULL) {
        wlr_output_state_set_mode(&state, mode);
    }

    /* Enable adaptive sync (variable refresh rate) if the output supports it.
     * This reduces perceived stutter during the Pulse Beat Ambient animation
     * on displays with VRR support. Non-fatal if unsupported. */
    if (wlr_output->adaptive_sync_supported) {
        wlr_output_state_set_adaptive_sync_enabled(&state, true);
    }

    /* Commit the initial output configuration */
    if (!wlr_output_commit_state(wlr_output, &state)) {
        wlr_log(WLR_ERROR, "Failed to set output mode for %s",
                wlr_output->name);
        wlr_output_state_finish(&state);
        return;
    }
    wlr_output_state_finish(&state);

    /* Allocate per-output state */
    struct pulse_output *output = calloc(1, sizeof(*output));
    if (!output) {
        wlr_log(WLR_ERROR, "Out of memory allocating pulse_output");
        return;
    }
    output->server = server;
    output->wlr_output = wlr_output;

    /* Register frame / state / destroy listeners */
    output->frame.notify = output_handle_frame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);

    output->request_state.notify = output_handle_request_state;
    wl_signal_add(&wlr_output->events.request_state, &output->request_state);

    output->destroy.notify = output_handle_destroy;
    wl_signal_add(&wlr_output->events.destroy, &output->destroy);

    wl_list_insert(&server->outputs, &output->link);

    /* Add this output to the layout, positioning it automatically to the
     * right of any existing outputs. A future Milestone will expose a
     * proper output-arrangement UI; for now auto-layout is correct. */
    struct wlr_output_layout_output *layout_output =
        wlr_output_layout_add_auto(server->output_layout, wlr_output);

    /* Create the wlr_scene_output binding so the scene graph knows about
     * this physical output and can render into it. */
    struct wlr_scene_output *scene_output =
        wlr_scene_output_create(server->scene, wlr_output);

    wlr_scene_output_layout_add_output(server->scene_layout, layout_output,
                                        scene_output);

    /* Set the background colour for this output's scene.
     * Named array required — compound literal float[] in a function arg
     * triggers -Wpedantic under -Werror with some C17 compilers. */
    static const float pulse_void_color[4] = {
        PULSE_VOID_R, PULSE_VOID_G, PULSE_VOID_B, PULSE_VOID_A
    };
    int bg_w = (mode != NULL) ? mode->width  : wlr_output->width;
    int bg_h = (mode != NULL) ? mode->height : wlr_output->height;

    struct wlr_scene_rect *bg = wlr_scene_rect_create(
        &server->scene->tree, bg_w, bg_h, pulse_void_color);
    if (!bg) {
        wlr_log(WLR_ERROR, "Failed to create background rect for %s",
                wlr_output->name);
        /* Non-fatal — scene defaults to transparent/black without this */
    }

    wlr_log(WLR_INFO, "Output configured: %s %dx%d@%.2fHz%s",
            wlr_output->name,
            wlr_output->width,
            wlr_output->height,
            (mode ? (float)mode->refresh / 1000.0f : 0.0f),
            wlr_output->adaptive_sync_supported ? " (adaptive sync)" : "");
}
