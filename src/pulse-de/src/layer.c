/* SPDX-License-Identifier: MIT */
/**
 * layer.c — layer-shell surface management for PulseDE.
 *
 * See include/pulse-layer.h for the full design rationale and Z-order
 * documentation.
 *
 * This implements the compositor side of wlr-layer-shell-unstable-v1.
 * The Pulse Panel (src/pulse-panel/) is the first client of this protocol.
 *
 * Key design choices:
 *
 * 1. Scene tree per layer. We create four wlr_scene_tree children under
 *    the root scene tree. wlr_scene_layer_surface_v1_create() places
 *    a layer surface into the correct tree automatically based on the
 *    surface's declared layer. This gives correct compositing order
 *    without manual z-order management in this file.
 *
 * 2. Exclusive zones. A surface (like the Pulse Panel) can declare an
 *    exclusive zone — a number of pixels at a screen edge that are reserved
 *    and should not be used by tiled/maximised windows. layer_arrange()
 *    computes these zones and stores a usable_area box per output.
 *    Window placement (Milestone 6+) will respect this area.
 *
 * 3. layer_arrange() is called on every map/unmap/commit of a layer surface
 *    and on every output configuration change, so the zones stay correct.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "pulse-layer.h"
#include "pulse-output.h"
#include "pulse-server.h"

/* ---------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/** Return the scene tree for a given layer enum value. */
static struct wlr_scene_tree *scene_tree_for_layer(struct pulse_server *server,
        enum zwlr_layer_shell_v1_layer layer)
{
    switch (layer) {
    case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND: return server->layer_tree_background;
    case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:     return server->layer_tree_bottom;
    case ZWLR_LAYER_SHELL_V1_LAYER_TOP:        return server->layer_tree_top;
    case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:    return server->layer_tree_overlay;
    default:                                    return server->layer_tree_top;
    }
}

/* ---------------------------------------------------------------------------
 * layer_arrange — compute geometry and configure all surfaces on an output
 * ------------------------------------------------------------------------- */

void layer_arrange(struct pulse_server *server, struct pulse_output *output)
{
    if (!output || !output->wlr_output) {
        return;
    }

    int ow = output->wlr_output->width;
    int oh = output->wlr_output->height;

    /* usable_area starts as the full output; surfaces with exclusive zones
     * shrink it from each edge. The Window Manager (future milestone) will
     * use server->usable_area to place maximised windows. */
    struct wlr_box usable = {0, 0, ow, oh};

    /* Iterate every layer surface associated with this output.
     * Process layers in order: background, bottom, top, overlay. */
    static const enum zwlr_layer_shell_v1_layer layer_order[] = {
        ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND,
        ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM,
        ZWLR_LAYER_SHELL_V1_LAYER_TOP,
        ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
    };

    for (size_t li = 0; li < 4; li++) {
        struct pulse_layer_surface *lsurf;
        wl_list_for_each(lsurf, &server->layer_surfaces, link) {
            struct wlr_layer_surface_v1 *wls = lsurf->wlr_layer_surface;
            if (wls->output != output->wlr_output) continue;
            if (wls->current.layer != layer_order[li]) continue;
            if (!wls->mapped) continue;

            const struct wlr_layer_surface_v1_state *state = &wls->current;
            struct wlr_box bounds = usable;

            /* Compute the surface's desired size.
             * A zero desired_width/height means "fill the axis". */
            int sw = state->desired_width  ? (int)state->desired_width  : bounds.width;
            int sh = state->desired_height ? (int)state->desired_height : bounds.height;

            /* Resolve anchor + margin to get the actual position */
            int sx = bounds.x;
            int sy = bounds.y;

            uint32_t anchor = state->anchor;
            bool anchor_left   = anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
            bool anchor_right  = anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
            bool anchor_top    = anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
            bool anchor_bottom = anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;

            /* Horizontal position */
            if (anchor_left && anchor_right) {
                sx = bounds.x + state->margin.left;
                sw = bounds.width - state->margin.left - state->margin.right;
            } else if (anchor_right) {
                sx = bounds.x + bounds.width - sw - state->margin.right;
            } else if (anchor_left) {
                sx = bounds.x + state->margin.left;
            } else {
                /* Centred */
                sx = bounds.x + (bounds.width - sw) / 2;
            }

            /* Vertical position */
            if (anchor_top && anchor_bottom) {
                sy = bounds.y + state->margin.top;
                sh = bounds.height - state->margin.top - state->margin.bottom;
            } else if (anchor_bottom) {
                sy = bounds.y + bounds.height - sh - state->margin.bottom;
            } else if (anchor_top) {
                sy = bounds.y + state->margin.top;
            } else {
                sy = bounds.y + (bounds.height - sh) / 2;
            }

            /* Send configure with the resolved dimensions */
            wlr_layer_surface_v1_configure(wls, (uint32_t)sw, (uint32_t)sh);

            /* Position the scene node */
            wlr_scene_node_set_position(&lsurf->scene_tree->node, sx, sy);

            /* Shrink the usable area by the exclusive zone */
            int32_t excl = state->exclusive_zone;
            if (excl > 0) {
                if (anchor_bottom && !anchor_top) {
                    usable.height -= excl + state->margin.bottom;
                } else if (anchor_top && !anchor_bottom) {
                    usable.y      += excl + state->margin.top;
                    usable.height -= excl + state->margin.top;
                } else if (anchor_left && !anchor_right) {
                    usable.x     += excl + state->margin.left;
                    usable.width -= excl + state->margin.left;
                } else if (anchor_right && !anchor_left) {
                    usable.width -= excl + state->margin.right;
                }
            }
        }
    }

    /* Store the usable area so the WM can use it. Per-output for M5;
     * in a multi-monitor future this will be stored in pulse_output. */
    output->usable_area = usable;
}

/* ---------------------------------------------------------------------------
 * Layer surface event handlers
 * ------------------------------------------------------------------------- */

static void layer_surface_handle_map(struct wl_listener *listener, void *data)
{
    struct pulse_layer_surface *lsurf =
        wl_container_of(listener, lsurf, map);

    /* Find the output this surface is on and re-arrange */
    struct pulse_output *output;
    wl_list_for_each(output, &lsurf->server->outputs, link) {
        if (output->wlr_output == lsurf->wlr_layer_surface->output) {
            layer_arrange(lsurf->server, output);
            break;
        }
    }

    wlr_log(WLR_DEBUG, "Layer surface mapped: namespace=%s layer=%d",
            lsurf->wlr_layer_surface->namespace ?
                lsurf->wlr_layer_surface->namespace : "(null)",
            lsurf->wlr_layer_surface->current.layer);
}

static void layer_surface_handle_unmap(struct wl_listener *listener, void *data)
{
    struct pulse_layer_surface *lsurf =
        wl_container_of(listener, lsurf, unmap);

    struct pulse_output *output;
    wl_list_for_each(output, &lsurf->server->outputs, link) {
        if (output->wlr_output == lsurf->wlr_layer_surface->output) {
            layer_arrange(lsurf->server, output);
            break;
        }
    }
}

static void layer_surface_handle_commit(struct wl_listener *listener, void *data)
{
    struct pulse_layer_surface *lsurf =
        wl_container_of(listener, lsurf, commit);

    if (lsurf->wlr_layer_surface->mapped) {
        struct pulse_output *output;
        wl_list_for_each(output, &lsurf->server->outputs, link) {
            if (output->wlr_output == lsurf->wlr_layer_surface->output) {
                layer_arrange(lsurf->server, output);
                break;
            }
        }
    }
}

static void layer_surface_handle_destroy(struct wl_listener *listener,
                                          void *data)
{
    struct pulse_layer_surface *lsurf =
        wl_container_of(listener, lsurf, destroy);

    wl_list_remove(&lsurf->map.link);
    wl_list_remove(&lsurf->unmap.link);
    wl_list_remove(&lsurf->commit.link);
    wl_list_remove(&lsurf->destroy.link);
    wl_list_remove(&lsurf->link);

    free(lsurf);
}

/* ---------------------------------------------------------------------------
 * New layer surface — called when a client creates a layer shell surface
 * ------------------------------------------------------------------------- */

static void layer_handle_new_surface(struct wl_listener *listener, void *data)
{
    struct pulse_server *server =
        wl_container_of(listener, server, new_layer_surface);
    struct wlr_layer_surface_v1 *wlr_layer_surface = data;

    /* If the client did not specify an output, assign the first available */
    if (!wlr_layer_surface->output) {
        struct pulse_output *output =
            wl_container_of(server->outputs.next, output, link);
        if (!output) {
            wlr_log(WLR_ERROR, "No output available for layer surface");
            wlr_layer_surface_v1_destroy(wlr_layer_surface);
            return;
        }
        wlr_layer_surface->output = output->wlr_output;
    }

    struct pulse_layer_surface *lsurf = calloc(1, sizeof(*lsurf));
    if (!lsurf) {
        wlr_log(WLR_ERROR, "Out of memory allocating pulse_layer_surface");
        wlr_layer_surface_v1_destroy(wlr_layer_surface);
        return;
    }
    lsurf->server = server;
    lsurf->wlr_layer_surface = wlr_layer_surface;

    /* Place into the correct scene layer tree */
    struct wlr_scene_tree *layer_tree =
        scene_tree_for_layer(server, wlr_layer_surface->current.layer);

    lsurf->scene_layer_surface =
        wlr_scene_layer_surface_v1_create(layer_tree, wlr_layer_surface);
    if (!lsurf->scene_layer_surface) {
        wlr_log(WLR_ERROR, "Failed to create scene layer surface");
        free(lsurf);
        return;
    }
    lsurf->scene_tree = lsurf->scene_layer_surface->tree;

    /* Wire event listeners */
    lsurf->map.notify = layer_surface_handle_map;
    wl_signal_add(&wlr_layer_surface->surface->events.map, &lsurf->map);

    lsurf->unmap.notify = layer_surface_handle_unmap;
    wl_signal_add(&wlr_layer_surface->surface->events.unmap, &lsurf->unmap);

    lsurf->commit.notify = layer_surface_handle_commit;
    wl_signal_add(&wlr_layer_surface->surface->events.commit, &lsurf->commit);

    lsurf->destroy.notify = layer_surface_handle_destroy;
    wl_signal_add(&wlr_layer_surface->events.destroy, &lsurf->destroy);

    wl_list_insert(&server->layer_surfaces, &lsurf->link);

    /* Send an initial configure so the client can commit its first frame */
    struct pulse_output *output;
    wl_list_for_each(output, &server->outputs, link) {
        if (output->wlr_output == wlr_layer_surface->output) {
            layer_arrange(server, output);
            break;
        }
    }

    wlr_log(WLR_DEBUG, "New layer surface: namespace=%s",
            wlr_layer_surface->namespace ?
                wlr_layer_surface->namespace : "(null)");
}

/* ---------------------------------------------------------------------------
 * layer_init
 * ------------------------------------------------------------------------- */

bool layer_init(struct pulse_server *server)
{
    /* Initialise the layer surfaces list */
    wl_list_init(&server->layer_surfaces);

    /* Create the four layer scene trees as children of the root scene tree.
     * Order matters: nodes added later are rendered on top. We create them
     * in bottom-to-top order so the Z-order is correct automatically. */
    server->layer_tree_background =
        wlr_scene_tree_create(&server->scene->tree);
    server->layer_tree_bottom =
        wlr_scene_tree_create(&server->scene->tree);
    /* XDG windows are added directly to server->scene->tree and will
     * therefore sit between bottom and top. bg_rect is lowered to bottom
     * in output.c, so bg is: bg_rect < bottom < windows < top < overlay */
    server->layer_tree_top =
        wlr_scene_tree_create(&server->scene->tree);
    server->layer_tree_overlay =
        wlr_scene_tree_create(&server->scene->tree);

    if (!server->layer_tree_background || !server->layer_tree_bottom ||
        !server->layer_tree_top        || !server->layer_tree_overlay) {
        wlr_log(WLR_ERROR, "Failed to create layer scene trees");
        return false;
    }

    /* Create the layer shell protocol handler */
    server->layer_shell = wlr_layer_shell_v1_create(server->wl_display, 4);
    if (!server->layer_shell) {
        wlr_log(WLR_ERROR, "Failed to create layer shell");
        return false;
    }

    server->new_layer_surface.notify = layer_handle_new_surface;
    wl_signal_add(&server->layer_shell->events.new_surface,
                  &server->new_layer_surface);

    wlr_log(WLR_INFO, "Layer shell initialised (version 4)");
    return true;
}
