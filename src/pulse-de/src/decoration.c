/* SPDX-License-Identifier: MIT */
/**
 * decoration.c — server-side window decoration for PulseDE.
 *
 * See include/pulse-decoration.h for the full design rationale.
 *
 * Design token reference (from docs/design-system/01-motion.md and
 * the frozen theme spec — do not change these values here; when the
 * Theme Engine is implemented in Milestone 6 it will drive them over IPC):
 *
 *   --pulse-signal:   #FF2438  → focused top border
 *   --pulse-line:     #2A2426  → unfocused border (all sides)
 *   --pulse-surface-2: #181517  → focused side/bottom borders
 *
 * Border geometry (Milestone 2 — titlebar is Milestone 3):
 *   top:    BORDER_TOP_PX    = 2px (focus accent; 1px when unfocused)
 *   sides:  BORDER_SIDE_PX   = 1px
 *   bottom: BORDER_BOTTOM_PX = 1px
 */

#include <stdlib.h>
#include <string.h>

#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/util/log.h>

#include "pulse-decoration.h"
#include "pulse-server.h"

/* ---------------------------------------------------------------------------
 * Design tokens — frozen values matching the theme spec.
 * These are compile-time constants until the Theme Engine (Milestone 6)
 * provides them at runtime via the Settings/Theme IPC API.
 * ---------------------------------------------------------------------------*/

/* --pulse-signal #FF2438 — focused top border */
static const float COLOR_SIGNAL[4]   = { 1.0f,  0.141f, 0.220f, 1.0f };
/* --pulse-line   #2A2426 — unfocused border (all sides) */
static const float COLOR_LINE[4]     = { 0.165f, 0.141f, 0.149f, 1.0f };
/* --pulse-surface-2 #181517 — focused side/bottom borders */
static const float COLOR_SURFACE2[4] = { 0.094f, 0.082f, 0.090f, 1.0f };

#define BORDER_TOP_PX_FOCUSED   2
#define BORDER_TOP_PX_UNFOCUSED 1
#define BORDER_SIDE_PX          1
#define BORDER_BOTTOM_PX        1

/* ---------------------------------------------------------------------------
 * xdg-decoration protocol handlers
 * ------------------------------------------------------------------------- */

static void decoration_handle_new_toplevel_decoration(
    struct wl_listener *listener, void *data)
{
    struct wlr_xdg_toplevel_decoration_v1 *decoration = data;

    /* Unconditionally request SERVER mode.
     * PulseDE draws all window decorations; clients must not draw their own.
     * This is the architectural guarantee behind Pulse HIG §1.1's requirement
     * that window controls are always in the same position in every app. */
    wlr_xdg_toplevel_decoration_v1_set_mode(
        decoration,
        WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
}

/* ---------------------------------------------------------------------------
 * decoration_init
 * ------------------------------------------------------------------------- */

bool decoration_init(struct pulse_server *server)
{
    server->decoration_mgr =
        wlr_xdg_decoration_manager_v1_create(server->wl_display);
    if (!server->decoration_mgr) {
        wlr_log(WLR_ERROR, "Failed to create xdg-decoration manager");
        return false;
    }

    /* Store the listener as a static — it lives for the session lifetime,
     * same as the manager. The manager is owned by wl_display. */
    static struct wl_listener new_decoration;
    new_decoration.notify = decoration_handle_new_toplevel_decoration;
    wl_signal_add(&server->decoration_mgr->events.new_toplevel_decoration,
                  &new_decoration);

    wlr_log(WLR_DEBUG, "xdg-decoration manager created (SSD mode)");
    return true;
}

/* ---------------------------------------------------------------------------
 * Border creation, update, destruction
 * ------------------------------------------------------------------------- */

struct pulse_window_border *decoration_create_border(
    struct pulse_server *server,
    struct wlr_scene_tree *toplevel_tree,
    int width, int height, bool focused)
{
    (void)server; /* Not needed yet; will be used by Theme Engine in M6 */

    struct pulse_window_border *border = calloc(1, sizeof(*border));
    if (!border) {
        wlr_log(WLR_ERROR, "Out of memory allocating pulse_window_border");
        return NULL;
    }

    int top_h = focused ? BORDER_TOP_PX_FOCUSED : BORDER_TOP_PX_UNFOCUSED;
    const float *top_color = focused ? COLOR_SIGNAL : COLOR_LINE;

    /* All border rects are children of the toplevel's scene tree so they
     * move and stack with it automatically. They are positioned relative
     * to the toplevel tree's origin (top-left corner of the window). */

    border->top = wlr_scene_rect_create(toplevel_tree,
                                         width, top_h, top_color);
    border->left = wlr_scene_rect_create(toplevel_tree,
                                          BORDER_SIDE_PX, height, COLOR_LINE);
    border->right = wlr_scene_rect_create(toplevel_tree,
                                           BORDER_SIDE_PX, height, COLOR_LINE);
    border->bottom = wlr_scene_rect_create(toplevel_tree,
                                            width, BORDER_BOTTOM_PX, COLOR_LINE);

    if (!border->top || !border->left || !border->right || !border->bottom) {
        wlr_log(WLR_ERROR, "Failed to create one or more border rects");
        decoration_destroy_border(border);
        return NULL;
    }

    /* Position each rect. wlr_scene nodes default to (0,0) relative to
     * their parent, so we only need to move the ones that aren't at (0,0). */

    /* top: x=0, y=0 — default, no move needed */

    /* left: x=0, y=0 — default, no move needed */

    /* right: x = width - BORDER_SIDE_PX, y=0 */
    wlr_scene_node_set_position(&border->right->node,
                                 width - BORDER_SIDE_PX, 0);

    /* bottom: x=0, y = height - BORDER_BOTTOM_PX */
    wlr_scene_node_set_position(&border->bottom->node,
                                 0, height - BORDER_BOTTOM_PX);

    return border;
}

void decoration_update_border(struct pulse_window_border *border,
                               int width, int height, bool focused)
{
    if (!border) {
        return;
    }

    int top_h = focused ? BORDER_TOP_PX_FOCUSED : BORDER_TOP_PX_UNFOCUSED;
    const float *top_color    = focused ? COLOR_SIGNAL   : COLOR_LINE;
    const float *side_color   = focused ? COLOR_SURFACE2 : COLOR_LINE;
    const float *bottom_color = focused ? COLOR_SURFACE2 : COLOR_LINE;

    /* Top border — resize, recolour, reposition (stays at 0,0) */
    wlr_scene_rect_set_size(border->top, width, top_h);
    wlr_scene_rect_set_color(border->top, top_color);

    /* Left border */
    wlr_scene_rect_set_size(border->left, BORDER_SIDE_PX, height);
    wlr_scene_rect_set_color(border->left, side_color);

    /* Right border */
    wlr_scene_rect_set_size(border->right, BORDER_SIDE_PX, height);
    wlr_scene_rect_set_color(border->right, side_color);
    wlr_scene_node_set_position(&border->right->node,
                                 width - BORDER_SIDE_PX, 0);

    /* Bottom border */
    wlr_scene_rect_set_size(border->bottom, width, BORDER_BOTTOM_PX);
    wlr_scene_rect_set_color(border->bottom, bottom_color);
    wlr_scene_node_set_position(&border->bottom->node,
                                 0, height - BORDER_BOTTOM_PX);
}

void decoration_destroy_border(struct pulse_window_border *border)
{
    if (!border) {
        return;
    }
    /* wlr_scene_node_destroy() removes the node from the scene graph and
     * frees the rect — safe to call on NULL (it's a no-op). */
    if (border->top)    { wlr_scene_node_destroy(&border->top->node);    }
    if (border->left)   { wlr_scene_node_destroy(&border->left->node);   }
    if (border->right)  { wlr_scene_node_destroy(&border->right->node);  }
    if (border->bottom) { wlr_scene_node_destroy(&border->bottom->node); }
    free(border);
}
