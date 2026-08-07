/* SPDX-License-Identifier: MIT */
/**
 * cursor.c — cursor and pointer focus management for PulseDE.
 *
 * Handles:
 *  - Relative and absolute pointer motion (moving the hardware cursor)
 *  - Updating pointer focus based on which surface is under the cursor
 *  - Focus-on-click: clicking an unfocused window focuses it
 *  - Axis (scroll wheel) events forwarded to the focused client
 *  - Frame events (flush accumulated pointer state to clients)
 *  - Client-requested cursor image changes
 *  - Seat clipboard selection
 */

#include <stdlib.h>
#include <time.h>

#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/log.h>

#include "pulse-grab.h"
#include "pulse-server.h"
#include "pulse-xdg-shell.h"

/* ---------------------------------------------------------------------------
 * Pointer focus update
 * Called after any cursor movement to update which surface has pointer focus.
 * ------------------------------------------------------------------------- */
static void cursor_update_focus(struct pulse_server *server, uint32_t time_msec)
{
    double sx, sy;
    struct wlr_surface *surface = NULL;
    struct pulse_toplevel *toplevel = xdg_shell_toplevel_at(
        server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);

    if (surface != NULL) {
        /* Only send pointer-enter when the focused surface has actually
         * changed. Sending enter unconditionally generates a redundant
         * enter+leave pair on every motion event, which can confuse clients
         * that track enter/leave to manage hover state or drag detection. */
        if (surface != server->seat->pointer_state.focused_surface) {
            wlr_seat_pointer_notify_enter(server->seat, surface, sx, sy);
        } else {
            wlr_seat_pointer_notify_motion(server->seat, time_msec, sx, sy);
        }
    } else {
        /* No surface under the cursor: restore the default cursor image and
         * clear pointer focus so the client receives a leave event. */
        wlr_cursor_set_xcursor(server->cursor, server->xcursor_mgr, "default");
        wlr_seat_pointer_clear_focus(server->seat);
    }

    (void)toplevel; /* toplevel pointer used in future milestones for
                     * hover-based focus-follows-pointer policy */
}

/* ---------------------------------------------------------------------------
 * Relative motion (mouse move)
 * ------------------------------------------------------------------------- */
void cursor_handle_motion(struct wl_listener *listener, void *data)
{
    struct pulse_server *server =
        wl_container_of(listener, server, cursor_motion);
    struct wlr_pointer_motion_event *event = data;

    wlr_cursor_move(server->cursor, &event->pointer->base,
                    event->delta_x, event->delta_y);

    if (grab_is_active(&server->grab)) {
        grab_update(server, event->time_msec);
    } else {
        cursor_update_focus(server, event->time_msec);
    }
}

/* ---------------------------------------------------------------------------
 * Absolute motion (touchpad, drawing tablet, nested compositor)
 * ------------------------------------------------------------------------- */
void cursor_handle_motion_absolute(struct wl_listener *listener, void *data)
{
    struct pulse_server *server =
        wl_container_of(listener, server, cursor_motion_absolute);
    struct wlr_pointer_motion_absolute_event *event = data;

    wlr_cursor_warp_absolute(server->cursor, &event->pointer->base,
                              event->x, event->y);

    if (grab_is_active(&server->grab)) {
        grab_update(server, event->time_msec);
    } else {
        cursor_update_focus(server, event->time_msec);
    }
}

/* ---------------------------------------------------------------------------
 * Button (mouse click)
 * On press: if the click lands on an unfocused toplevel, focus it first.
 * ------------------------------------------------------------------------- */
void cursor_handle_button(struct wl_listener *listener, void *data)
{
    struct pulse_server *server =
        wl_container_of(listener, server, cursor_button);
    struct wlr_pointer_button_event *event = data;

    /* wlroots 0.17: wlr_pointer_button_event.state is enum wlr_button_state
     * (WLR_BUTTON_PRESSED/RELEASED), not the wl_pointer protocol enum. */
    if (event->state == WLR_BUTTON_RELEASED) {
        /* Any button release ends an active grab, regardless of which
         * button it was. This prevents a stuck grab if the user releases
         * a different button than the one that started the grab. */
        if (grab_is_active(&server->grab)) {
            grab_end(server);
            return;
        }
    }

    /* Forward the button event to the focused client */
    wlr_seat_pointer_notify_button(server->seat, event->time_msec,
                                    event->button, event->state);

    if (event->state == WLR_BUTTON_PRESSED) {
        /* Focus-on-click: find the toplevel under the cursor and focus it. */
        double sx, sy;
        struct wlr_surface *surface = NULL;
        struct pulse_toplevel *toplevel = xdg_shell_toplevel_at(
            server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);

        xdg_shell_focus_toplevel(server, toplevel);
    }
}

/* ---------------------------------------------------------------------------
 * Scroll wheel (axis)
 * ------------------------------------------------------------------------- */
void cursor_handle_axis(struct wl_listener *listener, void *data)
{
    struct pulse_server *server =
        wl_container_of(listener, server, cursor_axis);
    struct wlr_pointer_axis_event *event = data;

    /* wlroots 0.17: no relative_direction argument (added in 0.18). */
    wlr_seat_pointer_notify_axis(server->seat, event->time_msec,
                                  event->orientation, event->delta,
                                  event->delta_discrete, event->source);
}

/* ---------------------------------------------------------------------------
 * Frame — flush accumulated pointer events to clients
 * ------------------------------------------------------------------------- */
void cursor_handle_frame(struct wl_listener *listener, void *data)
{
    struct pulse_server *server =
        wl_container_of(listener, server, cursor_frame);
    wlr_seat_pointer_notify_frame(server->seat);
}

/* ---------------------------------------------------------------------------
 * Client-requested cursor image change
 * A client (e.g. a text editor showing an I-beam) requests a different cursor.
 * We honour this only if the requesting surface currently has pointer focus.
 * ------------------------------------------------------------------------- */
void cursor_handle_request_set_cursor(struct wl_listener *listener, void *data)
{
    struct pulse_server *server =
        wl_container_of(listener, server, request_set_cursor);
    struct wlr_seat_pointer_request_set_cursor_event *event = data;

    struct wlr_seat_client *focused_client =
        server->seat->pointer_state.focused_client;

    /* Only accept cursor changes from the currently-focused client */
    if (focused_client != event->seat_client) {
        return;
    }

    wlr_cursor_set_surface(server->cursor, event->surface, event->hotspot_x,
                            event->hotspot_y);
}

/* ---------------------------------------------------------------------------
 * Seat clipboard selection
 * ------------------------------------------------------------------------- */
void seat_handle_request_set_selection(struct wl_listener *listener, void *data)
{
    struct pulse_server *server =
        wl_container_of(listener, server, request_set_selection);
    struct wlr_seat_request_set_selection_event *event = data;
    wlr_seat_set_selection(server->seat, event->source, event->serial);
}
