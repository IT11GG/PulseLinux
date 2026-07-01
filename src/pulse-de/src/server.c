/* SPDX-License-Identifier: MIT */
/**
 * server.c — PulseDE compositor initialisation and teardown.
 *
 * server_init() brings up every wlroots subsystem in the correct order:
 *   display → backend → renderer → allocator → scene → protocols →
 *   output-layout → seat/cursor → event listeners → socket
 *
 * server_finish() tears them down in strict reverse order.
 *
 * Rule: no subsystem is used before it is initialised; every subsystem is
 * destroyed before its dependencies. This file is the single place that
 * enforces that ordering — individual subsystem files (output.c, input.c,
 * xdg-shell.c) only see the already-initialised server struct.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#include "pulse-server.h"
#include "pulse-output.h"
#include "pulse-input.h"
#include "pulse-xdg-shell.h"

/* ---------------------------------------------------------------------------
 * Cursor event handlers (thin trampolines into cursor.c)
 * These are defined in cursor.c; declared here to wire the listeners.
 * ------------------------------------------------------------------------- */
void cursor_handle_motion(struct wl_listener *listener, void *data);
void cursor_handle_motion_absolute(struct wl_listener *listener, void *data);
void cursor_handle_button(struct wl_listener *listener, void *data);
void cursor_handle_axis(struct wl_listener *listener, void *data);
void cursor_handle_frame(struct wl_listener *listener, void *data);
void cursor_handle_request_set_cursor(struct wl_listener *listener, void *data);
void seat_handle_request_set_selection(struct wl_listener *listener, void *data);

/* ---------------------------------------------------------------------------
 * server_init
 * ------------------------------------------------------------------------- */
bool server_init(struct pulse_server *server)
{
    assert(server != NULL);

    /* Initialise list heads */
    wl_list_init(&server->outputs);
    wl_list_init(&server->toplevels);
    wl_list_init(&server->keyboards);

    /* ---- 1. Wayland display -------------------------------------------- */
    server->wl_display = wl_display_create();
    if (!server->wl_display) {
        wlr_log(WLR_ERROR, "Failed to create Wayland display");
        return false;
    }
    server->event_loop = wl_display_get_event_loop(server->wl_display);

    /* ---- 2. wlroots backend (auto-detect: DRM/KMS, Wayland, X11) ------- */
    server->backend = wlr_backend_autocreate(server->event_loop, NULL);
    if (!server->backend) {
        wlr_log(WLR_ERROR, "Failed to create wlroots backend");
        goto err_display;
    }

    /* ---- 3. Renderer (OpenGL ES via EGL, or Pixman software fallback) -- */
    server->renderer = wlr_renderer_autocreate(server->backend);
    if (!server->renderer) {
        wlr_log(WLR_ERROR, "Failed to create wlroots renderer");
        goto err_backend;
    }
    wlr_renderer_init_wl_display(server->renderer, server->wl_display);

    /* ---- 4. Allocator (picks GBM / Vulkan / shm based on renderer) ----- */
    server->allocator = wlr_allocator_autocreate(server->backend,
                                                   server->renderer);
    if (!server->allocator) {
        wlr_log(WLR_ERROR, "Failed to create wlroots allocator");
        goto err_renderer;
    }

    /* ---- 5. Scene graph ------------------------------------------------ */
    server->scene = wlr_scene_create();
    if (!server->scene) {
        wlr_log(WLR_ERROR, "Failed to create wlroots scene");
        goto err_allocator;
    }

    /* ---- 6. Output layout (tracks monitor positions/transforms) -------- */
    server->output_layout = wlr_output_layout_create(server->wl_display);
    if (!server->output_layout) {
        wlr_log(WLR_ERROR, "Failed to create output layout");
        goto err_scene;
    }

    /* Connect the scene to the output layout so wlr_scene handles
     * damage tracking, scaling, and presentation automatically. */
    server->scene_layout = wlr_scene_attach_output_layout(
        server->scene, server->output_layout);
    if (!server->scene_layout) {
        wlr_log(WLR_ERROR, "Failed to attach scene to output layout");
        goto err_output_layout;
    }

    /* ---- 7. Core Wayland protocols ------------------------------------- */
    server->compositor = wlr_compositor_create(server->wl_display, 6,
                                                server->renderer);
    if (!server->compositor) {
        wlr_log(WLR_ERROR, "Failed to create wlr_compositor");
        goto err_output_layout;
    }

    server->subcompositor = wlr_subcompositor_create(server->wl_display);
    if (!server->subcompositor) {
        wlr_log(WLR_ERROR, "Failed to create wlr_subcompositor");
        goto err_output_layout;
    }

    server->data_device_mgr =
        wlr_data_device_manager_create(server->wl_display);
    if (!server->data_device_mgr) {
        wlr_log(WLR_ERROR, "Failed to create data device manager");
        goto err_output_layout;
    }

    /* ---- 8. XDG shell -------------------------------------------------- */
    server->xdg_shell = wlr_xdg_shell_create(server->wl_display, 3);
    if (!server->xdg_shell) {
        wlr_log(WLR_ERROR, "Failed to create XDG shell");
        goto err_output_layout;
    }

    server->new_xdg_toplevel.notify = xdg_shell_handle_new_toplevel;
    wl_signal_add(&server->xdg_shell->events.new_toplevel,
                  &server->new_xdg_toplevel);

    server->new_xdg_popup.notify = xdg_shell_handle_new_popup;
    wl_signal_add(&server->xdg_shell->events.new_popup,
                  &server->new_xdg_popup);

    /* ---- 9. Seat (logical input device group) -------------------------- */
    server->seat = wlr_seat_create(server->wl_display, "seat0");
    if (!server->seat) {
        wlr_log(WLR_ERROR, "Failed to create Wayland seat");
        goto err_output_layout;
    }

    server->request_set_cursor.notify = cursor_handle_request_set_cursor;
    wl_signal_add(&server->seat->events.request_set_cursor,
                  &server->request_set_cursor);

    server->request_set_selection.notify = seat_handle_request_set_selection;
    wl_signal_add(&server->seat->events.request_set_selection,
                  &server->request_set_selection);

    /* ---- 10. Cursor ---------------------------------------------------- */
    server->cursor = wlr_cursor_create();
    if (!server->cursor) {
        wlr_log(WLR_ERROR, "Failed to create cursor");
        goto err_output_layout;
    }
    wlr_cursor_attach_output_layout(server->cursor, server->output_layout);

    /* Xcursor manager loads cursor themes from the host system.
     * "default" uses the system cursor theme; size 24 matches the Pulse
     * cursor specification (24px base grid, Icon System §2). */
    server->xcursor_mgr = wlr_xcursor_manager_create("default", 24);
    if (!server->xcursor_mgr) {
        wlr_log(WLR_ERROR, "Failed to create xcursor manager");
        goto err_cursor;
    }
    wlr_xcursor_manager_load(server->xcursor_mgr, 1.0f);

    /* Cursor event listeners */
    server->cursor_motion.notify = cursor_handle_motion;
    wl_signal_add(&server->cursor->events.motion, &server->cursor_motion);

    server->cursor_motion_absolute.notify = cursor_handle_motion_absolute;
    wl_signal_add(&server->cursor->events.motion_absolute,
                  &server->cursor_motion_absolute);

    server->cursor_button.notify = cursor_handle_button;
    wl_signal_add(&server->cursor->events.button, &server->cursor_button);

    server->cursor_axis.notify = cursor_handle_axis;
    wl_signal_add(&server->cursor->events.axis, &server->cursor_axis);

    server->cursor_frame.notify = cursor_handle_frame;
    wl_signal_add(&server->cursor->events.frame, &server->cursor_frame);

    /* ---- 11. Input device listener ------------------------------------ */
    server->new_input.notify = input_handle_new_input;
    wl_signal_add(&server->backend->events.new_input, &server->new_input);

    /* ---- 12. Output listener ------------------------------------------ */
    server->new_output.notify = output_handle_new;
    wl_signal_add(&server->backend->events.new_output, &server->new_output);

    wlr_log(WLR_INFO, "PulseDE compositor initialised");
    return true;

    /* ---- Error paths (reverse-init cleanup) ---------------------------- */
err_cursor:
    wlr_cursor_destroy(server->cursor);
    server->cursor = NULL;
err_output_layout:
    /* wlr_output_layout and everything below it is destroyed by
     * wl_display_destroy() via the display's destroy listeners, so we only
     * need to handle what wlroots does NOT automatically clean up. */
err_scene:
    /* wlr_scene is cleaned up via the display */
err_allocator:
    wlr_allocator_destroy(server->allocator);
    server->allocator = NULL;
err_renderer:
    wlr_renderer_destroy(server->renderer);
    server->renderer = NULL;
err_backend:
    wlr_backend_destroy(server->backend);
    server->backend = NULL;
err_display:
    wl_display_destroy(server->wl_display);
    server->wl_display = NULL;
    return false;
}

/* ---------------------------------------------------------------------------
 * server_run
 * ------------------------------------------------------------------------- */
bool server_run(struct pulse_server *server)
{
    assert(server != NULL);
    assert(server->wl_display != NULL);
    assert(server->backend != NULL);

    /* Create the Wayland socket that clients connect to.
     * wl_display_add_socket_auto() picks "wayland-N" for the smallest
     * available N and sets WAYLAND_DISPLAY in the environment. */
    const char *socket = wl_display_add_socket_auto(server->wl_display);
    if (!socket) {
        wlr_log(WLR_ERROR, "Failed to create Wayland socket");
        return false;
    }
    wlr_log(WLR_INFO, "PulseDE running on WAYLAND_DISPLAY=%s", socket);

    /* Announce the socket to any child processes we might spawn */
    setenv("WAYLAND_DISPLAY", socket, true);

    /* Start the backend — this opens the DRM/KMS device, or connects to
     * the parent Wayland/X11 compositor, depending on auto-detection. */
    if (!wlr_backend_start(server->backend)) {
        wlr_log(WLR_ERROR, "Failed to start backend");
        return false;
    }

    /* Set the initial cursor image */
    wlr_xcursor_manager_set_cursor_image(server->xcursor_mgr, "default",
                                          server->cursor);

    /* Block here until wl_display_terminate() is called (from a signal
     * handler or a protocol request) or the backend signals destruction. */
    wl_display_run(server->wl_display);

    return true;
}

/* ---------------------------------------------------------------------------
 * server_finish
 * ------------------------------------------------------------------------- */
void server_finish(struct pulse_server *server)
{
    if (!server || !server->wl_display) {
        return;
    }

    wlr_log(WLR_DEBUG, "Destroying PulseDE compositor");

    /* wl_display_destroy_clients() sends wl_display.delete_id to every
     * connected client, giving them a chance to clean up before we pull
     * the rug out from under them. */
    wl_display_destroy_clients(server->wl_display);

    /* Subsystem teardown in reverse-init order.
     * Most wlroots objects register a destroy listener on the display and
     * are cleaned up by wl_display_destroy() — but we explicitly destroy
     * the objects we allocated manually. */
    if (server->xcursor_mgr) {
        wlr_xcursor_manager_destroy(server->xcursor_mgr);
    }
    if (server->cursor) {
        wlr_cursor_destroy(server->cursor);
    }
    if (server->allocator) {
        wlr_allocator_destroy(server->allocator);
    }
    if (server->renderer) {
        wlr_renderer_destroy(server->renderer);
    }
    if (server->backend) {
        wlr_backend_destroy(server->backend);
    }

    /* Final display teardown — destroys everything registered with the
     * display's destroy listeners (output_layout, scene, protocols). */
    wl_display_destroy(server->wl_display);

    wlr_log(WLR_INFO, "PulseDE compositor destroyed");
}
