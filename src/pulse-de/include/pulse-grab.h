/* SPDX-License-Identifier: MIT */
/**
 * pulse-grab.h — interactive window grab state for PulseDE.
 *
 * A "grab" is the compositor-side mechanism for interactive window move and
 * resize operations. When a client sends xdg_toplevel.move or
 * xdg_toplevel.resize (typically triggered by the user pressing a titlebar
 * or dragging a resize edge), the compositor enters grab mode:
 *
 *  - All cursor motion events are redirected from pointer-focus updates
 *    to window repositioning or resizing.
 *  - The cursor image changes to reflect the grab type (grab, n-resize, etc.)
 *  - The grab ends on the next button-release event.
 *
 * Only one grab can be active at a time. Attempting to start a new grab
 * while one is active (e.g., two simultaneous buttons) silently no-ops.
 *
 * This file declares the grab state and the functions that drive it.
 * cursor.c is the primary consumer — it checks grab state on every
 * pointer-motion event and dispatches to grab_update_move() or
 * grab_update_resize() instead of the normal cursor_update_focus() path.
 */

#pragma once

#include <stdint.h>
#include <wlr/types/wlr_xdg_shell.h>

struct pulse_server;
struct pulse_toplevel;

/**
 * pulse_grab_type — what kind of interactive operation is in progress.
 */
enum pulse_grab_type {
    PULSE_GRAB_NONE   = 0,
    PULSE_GRAB_MOVE   = 1,
    PULSE_GRAB_RESIZE = 2,
};

/**
 * pulse_grab — active grab state, stored directly in pulse_server.
 *
 * All fields are valid only when type != PULSE_GRAB_NONE.
 * Fields are set by grab_begin_*() and read by grab_update_*().
 */
struct pulse_grab {
    enum pulse_grab_type    type;

    struct pulse_toplevel  *toplevel;   /* window being moved/resized        */

    /* Cursor position at the moment the grab started, in layout coordinates */
    double                  grab_x;
    double                  grab_y;

    /* Window geometry at the moment the grab started */
    int                     win_x;
    int                     win_y;
    int                     win_w;
    int                     win_h;

    /* Resize edges bitmask (WLR_EDGE_* values from wlr/types/wlr_xdg_shell.h)
     * Only meaningful when type == PULSE_GRAB_RESIZE. */
    uint32_t                resize_edges;
};

/**
 * grab_begin_move() - start an interactive move on toplevel.
 *
 * Called from toplevel_handle_request_move(). Records the current cursor
 * and window positions, sets grab.type = PULSE_GRAB_MOVE, and changes
 * the cursor image to "grab".
 *
 * No-ops if a grab is already active.
 */
void grab_begin_move(struct pulse_server *server,
                     struct pulse_toplevel *toplevel,
                     uint32_t serial);

/**
 * grab_begin_resize() - start an interactive resize on toplevel.
 *
 * Called from toplevel_handle_request_resize(). Records geometry and which
 * edges are being dragged (edges bitmask from the xdg_toplevel.resize event).
 * Sets grab.type = PULSE_GRAB_RESIZE and updates cursor image.
 *
 * No-ops if a grab is already active.
 */
void grab_begin_resize(struct pulse_server *server,
                       struct pulse_toplevel *toplevel,
                       uint32_t serial,
                       uint32_t edges);

/**
 * grab_update() - process a cursor motion event during an active grab.
 *
 * Called from cursor_handle_motion() / cursor_handle_motion_absolute()
 * when grab.type != PULSE_GRAB_NONE. Computes the delta from grab_x/grab_y
 * and applies it as a position update (move) or configure request (resize).
 *
 * Must not be called when grab.type == PULSE_GRAB_NONE.
 */
void grab_update(struct pulse_server *server, uint32_t time_msec);

/**
 * grab_end() - end the current grab on button release.
 *
 * Resets grab.type to PULSE_GRAB_NONE, restores the default cursor image,
 * and re-enables normal pointer-focus updates. Safe to call when no grab
 * is active (no-ops in that case).
 */
void grab_end(struct pulse_server *server);

/**
 * grab_is_active() - returns true if a grab is currently in progress.
 */
static inline bool grab_is_active(const struct pulse_grab *grab)
{
    return grab->type != PULSE_GRAB_NONE;
}
