/* SPDX-License-Identifier: MIT */
/**
 * grab.c — interactive window move and resize for PulseDE.
 *
 * See include/pulse-grab.h for the full API contract.
 *
 * How move works:
 *   1. Client sends xdg_toplevel.move (e.g. user pressed titlebar).
 *   2. grab_begin_move() records the cursor position and window position.
 *   3. Every subsequent cursor-motion event calls grab_update(), which
 *      computes (cursor_now - cursor_at_grab_start) and adds that delta
 *      to the window's original position.
 *   4. grab_end() on button-release restores normal pointer handling.
 *
 * How resize works:
 *   1. Client sends xdg_toplevel.resize with an edges bitmask.
 *   2. grab_begin_resize() records cursor position, window geometry, and edges.
 *   3. grab_update() computes the delta and derives the new width/height
 *      based on which edges are being dragged, then sends a configure request
 *      to the client. The client commits the new size; xdg-shell.c's commit
 *      handler updates the border geometry.
 *   4. grab_end() on button-release.
 *
 * Minimum window size:
 *   We enforce a 120x80px floor. Without this, a user can drag a window
 *   to zero or negative size, which can crash clients that don't validate
 *   their own configure acknowledgements.
 */

#include <stdlib.h>
#include <string.h>

#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/edges.h>
#include <wlr/util/log.h>

#include "pulse-grab.h"
#include "pulse-server.h"
#include "pulse-xdg-shell.h"

/* Minimum window dimensions enforced during resize */
#define MIN_WIN_WIDTH  120
#define MIN_WIN_HEIGHT  80

/* ---------------------------------------------------------------------------
 * Cursor image helpers
 *
 * wlr_xcursor_manager_set_cursor_image() takes a cursor name string.
 * We use the standard XCursor names that all desktop themes provide.
 * ------------------------------------------------------------------------- */

static void set_cursor(struct pulse_server *server, const char *name)
{
    wlr_cursor_set_xcursor(server->cursor, server->xcursor_mgr, name);
}

/** Return the XCursor name for a given resize edges bitmask. */
static const char *cursor_name_for_resize_edges(uint32_t edges)
{
    if ((edges & WLR_EDGE_TOP) && (edges & WLR_EDGE_LEFT))   return "nw-resize";
    if ((edges & WLR_EDGE_TOP) && (edges & WLR_EDGE_RIGHT))  return "ne-resize";
    if ((edges & WLR_EDGE_BOTTOM) && (edges & WLR_EDGE_LEFT))  return "sw-resize";
    if ((edges & WLR_EDGE_BOTTOM) && (edges & WLR_EDGE_RIGHT)) return "se-resize";
    if (edges & WLR_EDGE_TOP)    return "n-resize";
    if (edges & WLR_EDGE_BOTTOM) return "s-resize";
    if (edges & WLR_EDGE_LEFT)   return "w-resize";
    if (edges & WLR_EDGE_RIGHT)  return "e-resize";
    return "se-resize"; /* fallback */
}

/* ---------------------------------------------------------------------------
 * grab_begin_move
 * ------------------------------------------------------------------------- */

void grab_begin_move(struct pulse_server *server,
                     struct pulse_toplevel *toplevel,
                     uint32_t serial)
{
    (void)serial; /* Serial is checked by wlroots before the event fires;
                   * we don't need to re-validate it here. */

    if (grab_is_active(&server->grab)) {
        /* Already grabbing — ignore. This can happen if the user presses
         * a second mouse button before releasing the first. */
        return;
    }

    /* Record the window's current scene-graph position.
     * wlr_scene_node_coords() fills (lx, ly) with the node's layout-space
     * coordinates, walking up through parent transforms. */
    int lx = 0, ly = 0;
    wlr_scene_node_coords(&toplevel->scene_tree->node, &lx, &ly);

    server->grab = (struct pulse_grab){
        .type     = PULSE_GRAB_MOVE,
        .toplevel = toplevel,
        .grab_x   = server->cursor->x,
        .grab_y   = server->cursor->y,
        .win_x    = lx,
        .win_y    = ly,
    };

    set_cursor(server, "grab");

    wlr_log(WLR_DEBUG, "grab: move started on window at (%d,%d)", lx, ly);
}

/* ---------------------------------------------------------------------------
 * grab_begin_resize
 * ------------------------------------------------------------------------- */

void grab_begin_resize(struct pulse_server *server,
                       struct pulse_toplevel *toplevel,
                       uint32_t serial,
                       uint32_t edges)
{
    (void)serial;

    if (grab_is_active(&server->grab)) {
        return;
    }

    /* Get the current window geometry from the xdg_surface.
     * wlr_xdg_surface_get_geometry() fills a box with the window's
     * logical size (excluding any CSD shadows the client might add). */
    struct wlr_box geo = {0};
    wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);

    int lx = 0, ly = 0;
    wlr_scene_node_coords(&toplevel->scene_tree->node, &lx, &ly);

    server->grab = (struct pulse_grab){
        .type         = PULSE_GRAB_RESIZE,
        .toplevel     = toplevel,
        .grab_x       = server->cursor->x,
        .grab_y       = server->cursor->y,
        .win_x        = lx,
        .win_y        = ly,
        .win_w        = geo.width,
        .win_h        = geo.height,
        .resize_edges = edges,
    };

    /* Set cursor to match the resize direction */
    set_cursor(server, cursor_name_for_resize_edges(edges));

    /* Notify the client it is being resized. This causes the client to
     * begin acknowledging configure events for the new size. */
    wlr_xdg_toplevel_set_resizing(toplevel->xdg_toplevel, true);

    wlr_log(WLR_DEBUG, "grab: resize started (edges=0x%x) on %dx%d window",
            edges, geo.width, geo.height);
}

/* ---------------------------------------------------------------------------
 * grab_update — called on every cursor motion event during a grab
 * ------------------------------------------------------------------------- */

void grab_update(struct pulse_server *server, uint32_t time_msec)
{
    (void)time_msec;

    struct pulse_grab *grab = &server->grab;
    struct pulse_toplevel *tl = grab->toplevel;

    if (!tl) {
        grab_end(server);
        return;
    }

    /* Delta from cursor position at grab start */
    double dx = server->cursor->x - grab->grab_x;
    double dy = server->cursor->y - grab->grab_y;

    switch (grab->type) {

    case PULSE_GRAB_MOVE: {
        /* New window position = original position + cursor delta */
        int new_x = grab->win_x + (int)dx;
        int new_y = grab->win_y + (int)dy;

        /* Clamp so the window can't be dragged fully off-screen.
         * Allow the window to go partially off the top but not so far
         * that the titlebar area (where the user grabbed) is unreachable. */
        if (new_y < 0) new_y = 0;

        wlr_scene_node_set_position(&tl->scene_tree->node, new_x, new_y);
        break;
    }

    case PULSE_GRAB_RESIZE: {
        uint32_t edges = grab->resize_edges;

        /* Start from the original geometry */
        int new_x = grab->win_x;
        int new_y = grab->win_y;
        int new_w = grab->win_w;
        int new_h = grab->win_h;

        /* Adjust geometry based on which edges are being dragged.
         *
         * For right/bottom edges: adding delta increases the dimension.
         * For left/top edges: adding delta shifts the window origin AND
         * reduces the dimension — the opposite corner stays fixed. */
        if (edges & WLR_EDGE_RIGHT) {
            new_w = grab->win_w + (int)dx;
        }
        if (edges & WLR_EDGE_BOTTOM) {
            new_h = grab->win_h + (int)dy;
        }
        if (edges & WLR_EDGE_LEFT) {
            new_w = grab->win_w - (int)dx;
            new_x = grab->win_x + (int)dx;
        }
        if (edges & WLR_EDGE_TOP) {
            new_h = grab->win_h - (int)dy;
            new_y = grab->win_y + (int)dy;
        }

        /* Enforce minimum window size. If the new dimension would be below
         * the minimum, clamp it AND adjust the origin so the fixed corner
         * doesn't jump. */
        if (new_w < MIN_WIN_WIDTH) {
            if (edges & WLR_EDGE_LEFT) {
                new_x = grab->win_x + grab->win_w - MIN_WIN_WIDTH;
            }
            new_w = MIN_WIN_WIDTH;
        }
        if (new_h < MIN_WIN_HEIGHT) {
            if (edges & WLR_EDGE_TOP) {
                new_y = grab->win_y + grab->win_h - MIN_WIN_HEIGHT;
            }
            new_h = MIN_WIN_HEIGHT;
        }

        /* Move the scene node to the new position */
        wlr_scene_node_set_position(&tl->scene_tree->node, new_x, new_y);

        /* Send a configure request to the client with the new dimensions.
         * The client will commit an updated surface at this size; that
         * commit triggers toplevel_handle_commit() which calls
         * decoration_update_border() to resize the border rects. */
        wlr_xdg_toplevel_set_size(tl->xdg_toplevel,
                                   (uint32_t)new_w,
                                   (uint32_t)new_h);
        break;
    }

    case PULSE_GRAB_NONE:
    default:
        /* Should not reach here — caller checks grab_is_active() first */
        break;
    }
}

/* ---------------------------------------------------------------------------
 * grab_end
 * ------------------------------------------------------------------------- */

void grab_end(struct pulse_server *server)
{
    if (!grab_is_active(&server->grab)) {
        return;
    }

    /* If we were resizing, tell the client the resize is done */
    if (server->grab.type == PULSE_GRAB_RESIZE && server->grab.toplevel) {
        wlr_xdg_toplevel_set_resizing(server->grab.toplevel->xdg_toplevel,
                                       false);
    }

    wlr_log(WLR_DEBUG, "grab: ended (type=%d)", server->grab.type);

    /* Reset grab state */
    memset(&server->grab, 0, sizeof(server->grab));
    server->grab.type = PULSE_GRAB_NONE;

    /* Restore default cursor */
    set_cursor(server, "default");
}
