/* SPDX-License-Identifier: MIT */
/**
 * xdg-shell.c — XDG shell window management for PulseDE.
 *
 * Implements:
 *  - new_toplevel: allocate pulse_toplevel, add to scene graph
 *  - map/unmap: show/hide windows
 *  - focus: enforce the PulseDE focus invariant (exactly one focused
 *    toplevel at any time — Pulse Window Manager spec §4)
 *  - move/resize request handling
 *  - popup support
 */

#include <assert.h>
#include <stdlib.h>

#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#include "pulse-server.h"
#include "pulse-xdg-shell.h"

/* ---------------------------------------------------------------------------
 * Focus
 * ------------------------------------------------------------------------- */
void xdg_shell_focus_toplevel(struct pulse_server *server,
                               struct pulse_toplevel *toplevel)
{
    /* The focus invariant: defocus the previously-focused toplevel before
     * setting focus on the new one. A wlr_seat holds exactly one focused
     * surface at a time, but we also maintain our own focused_toplevel
     * pointer so the shell can query it without walking the seat. */
    struct pulse_toplevel *prev = server->focused_toplevel;

    if (prev == toplevel) {
        return; /* already focused — nothing to do */
    }

    if (prev != NULL) {
        /* Tell the previously-focused window it no longer has focus.
         * wlr_xdg_toplevel_set_activated(false) causes the client to draw
         * its window as inactive (dimmed titlebar, etc.). */
        wlr_xdg_toplevel_set_activated(prev->xdg_toplevel, false);
    }

    server->focused_toplevel = toplevel;

    if (toplevel == NULL) {
        /* Clearing focus — release keyboard from any surface */
        wlr_seat_keyboard_notify_clear_focus(server->seat);
        return;
    }

    /* Raise the window to the top of the scene graph */
    wlr_scene_node_raise_to_top(&toplevel->scene_tree->node);

    /* Activate the toplevel (client draws active/focused state) */
    wlr_xdg_toplevel_set_activated(toplevel->xdg_toplevel, true);

    /* Transfer keyboard focus */
    struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(server->seat);
    if (keyboard != NULL) {
        wlr_seat_keyboard_notify_enter(server->seat,
                                        toplevel->xdg_toplevel->base->surface,
                                        keyboard->keycodes,
                                        keyboard->num_keycodes,
                                        &keyboard->modifiers);
    }
}

/* ---------------------------------------------------------------------------
 * Surface hit-testing
 * ------------------------------------------------------------------------- */
struct pulse_toplevel *xdg_shell_toplevel_at(struct pulse_server *server,
                                              double lx, double ly,
                                              struct wlr_surface **surface,
                                              double *sx, double *sy)
{
    /* wlr_scene_node_at() walks the scene tree from top to bottom and
     * returns the first node under the given layout-space coordinates. */
    struct wlr_scene_node *node =
        wlr_scene_node_at(&server->scene->tree.node, lx, ly, sx, sy);

    if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER) {
        return NULL;
    }

    struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
    struct wlr_scene_surface *scene_surface =
        wlr_scene_surface_try_from_buffer(scene_buffer);

    if (scene_surface == NULL) {
        return NULL;
    }

    *surface = scene_surface->surface;

    /* Walk up the scene tree to find the pulse_toplevel that owns this node */
    struct wlr_scene_tree *tree = node->parent;
    while (tree != NULL && tree->node.data == NULL) {
        tree = tree->node.parent;
    }

    if (tree == NULL) {
        return NULL;
    }

    return (struct pulse_toplevel *)tree->node.data;
}

/* ---------------------------------------------------------------------------
 * Toplevel event handlers
 * ------------------------------------------------------------------------- */
static void toplevel_handle_map(struct wl_listener *listener, void *data)
{
    /* A toplevel is "mapped" when the client is ready to display content.
     * We focus it on map to match the natural expectation that a newly
     * opened window receives keyboard focus. */
    struct pulse_toplevel *toplevel = wl_container_of(listener, toplevel, map);
    xdg_shell_focus_toplevel(toplevel->server, toplevel);
}

static void toplevel_handle_unmap(struct wl_listener *listener, void *data)
{
    struct pulse_toplevel *toplevel =
        wl_container_of(listener, toplevel, unmap);

    if (toplevel->server->focused_toplevel == toplevel) {
        /* Focus the next toplevel in the list, or clear focus if none */
        struct pulse_toplevel *next = NULL;
        struct pulse_toplevel *t;
        wl_list_for_each(t, &toplevel->server->toplevels, link) {
            if (t != toplevel) {
                next = t;
                break;
            }
        }
        xdg_shell_focus_toplevel(toplevel->server, next);
    }
}

static void toplevel_handle_commit(struct wl_listener *listener, void *data)
{
    /* Fired when the client commits a new surface state (new frame content,
     * geometry changes, etc.). wlr_scene handles damage propagation
     * automatically, so this handler only needs to act on geometry changes
     * such as the initial configure acknowledgement. */
    struct pulse_toplevel *toplevel =
        wl_container_of(listener, toplevel, commit);

    if (toplevel->xdg_toplevel->base->initial_commit) {
        /* First commit after creation — send the initial configure.
         * Passing 0,0 lets the client choose its own size. A future
         * Milestone will set the size based on the output geometry and
         * the window's desired placement zone. */
        wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, 0, 0);
    }
}

static void toplevel_handle_destroy(struct wl_listener *listener, void *data)
{
    struct pulse_toplevel *toplevel =
        wl_container_of(listener, toplevel, destroy);

    wl_list_remove(&toplevel->map.link);
    wl_list_remove(&toplevel->unmap.link);
    wl_list_remove(&toplevel->commit.link);
    wl_list_remove(&toplevel->destroy.link);
    wl_list_remove(&toplevel->request_move.link);
    wl_list_remove(&toplevel->request_resize.link);
    wl_list_remove(&toplevel->request_maximize.link);
    wl_list_remove(&toplevel->request_fullscreen.link);
    wl_list_remove(&toplevel->link);

    free(toplevel);
}

static void toplevel_handle_request_move(struct wl_listener *listener,
                                          void *data)
{
    /* Interactive window move — implemented in Milestone 3 (Window Manager).
     * For Milestone 1 we acknowledge the request without acting on it,
     * which is protocol-correct: the client sends a request and the
     * compositor decides whether and how to honour it. */
    struct pulse_toplevel *toplevel =
        wl_container_of(listener, toplevel, request_move);
    (void)toplevel;
    wlr_log(WLR_DEBUG, "request_move: not yet implemented");
}

static void toplevel_handle_request_resize(struct wl_listener *listener,
                                            void *data)
{
    /* Interactive window resize — implemented in Milestone 3. */
    struct pulse_toplevel *toplevel =
        wl_container_of(listener, toplevel, request_resize);
    (void)toplevel;
    wlr_log(WLR_DEBUG, "request_resize: not yet implemented");
}

static void toplevel_handle_request_maximize(struct wl_listener *listener,
                                              void *data)
{
    /* Maximize request — deny for now; Milestone 3 implements snap zones. */
    struct pulse_toplevel *toplevel =
        wl_container_of(listener, toplevel, request_maximize);
    wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
}

static void toplevel_handle_request_fullscreen(struct wl_listener *listener,
                                                void *data)
{
    /* Fullscreen request — deny for now. */
    struct pulse_toplevel *toplevel =
        wl_container_of(listener, toplevel, request_fullscreen);
    wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
}

/* ---------------------------------------------------------------------------
 * New toplevel — called when a client creates an application window
 * ------------------------------------------------------------------------- */
void xdg_shell_handle_new_toplevel(struct wl_listener *listener, void *data)
{
    struct pulse_server *server =
        wl_container_of(listener, server, new_xdg_toplevel);
    struct wlr_xdg_toplevel *xdg_toplevel = data;

    struct pulse_toplevel *toplevel = calloc(1, sizeof(*toplevel));
    if (!toplevel) {
        wlr_log(WLR_ERROR, "Out of memory allocating pulse_toplevel");
        return;
    }
    toplevel->server = server;
    toplevel->xdg_toplevel = xdg_toplevel;

    /* Place the window in the scene graph.
     * wlr_scene_xdg_surface_create() creates a scene subtree that tracks
     * the xdg_surface's surface tree automatically (subsurfaces, popups). */
    toplevel->scene_tree = wlr_scene_xdg_surface_create(
        &server->scene->tree, xdg_toplevel->base);
    if (!toplevel->scene_tree) {
        wlr_log(WLR_ERROR, "Failed to create scene tree for toplevel");
        free(toplevel);
        return;
    }
    /* Store a back-pointer in the scene node so xdg_shell_toplevel_at()
     * can walk up from any surface node to its owning pulse_toplevel. */
    toplevel->scene_tree->node.data = toplevel;

    /* Wire up all toplevel event listeners */
    toplevel->map.notify = toplevel_handle_map;
    wl_signal_add(&xdg_toplevel->base->surface->events.map, &toplevel->map);

    toplevel->unmap.notify = toplevel_handle_unmap;
    wl_signal_add(&xdg_toplevel->base->surface->events.unmap, &toplevel->unmap);

    toplevel->commit.notify = toplevel_handle_commit;
    wl_signal_add(&xdg_toplevel->base->surface->events.commit, &toplevel->commit);

    toplevel->destroy.notify = toplevel_handle_destroy;
    wl_signal_add(&xdg_toplevel->events.destroy, &toplevel->destroy);

    toplevel->request_move.notify = toplevel_handle_request_move;
    wl_signal_add(&xdg_toplevel->events.request_move, &toplevel->request_move);

    toplevel->request_resize.notify = toplevel_handle_request_resize;
    wl_signal_add(&xdg_toplevel->events.request_resize,
                  &toplevel->request_resize);

    toplevel->request_maximize.notify = toplevel_handle_request_maximize;
    wl_signal_add(&xdg_toplevel->events.request_maximize,
                  &toplevel->request_maximize);

    toplevel->request_fullscreen.notify = toplevel_handle_request_fullscreen;
    wl_signal_add(&xdg_toplevel->events.request_fullscreen,
                  &toplevel->request_fullscreen);

    wl_list_insert(&server->toplevels, &toplevel->link);

    wlr_log(WLR_DEBUG, "New toplevel: app_id=%s title=%s",
            xdg_toplevel->app_id ? xdg_toplevel->app_id : "(null)",
            xdg_toplevel->title  ? xdg_toplevel->title  : "(null)");
}

/* ---------------------------------------------------------------------------
 * New popup — called when a client creates a popup (context menu, tooltip)
 * ------------------------------------------------------------------------- */
static void popup_handle_commit(struct wl_listener *listener, void *data)
{
    struct pulse_popup *popup = wl_container_of(listener, popup, commit);
    if (popup->xdg_popup->base->initial_commit) {
        wlr_xdg_popup_unconstrain_from_box(popup->xdg_popup,
            &(struct wlr_box){0, 0, 4096, 4096});
    }
}

static void popup_handle_destroy(struct wl_listener *listener, void *data)
{
    struct pulse_popup *popup = wl_container_of(listener, popup, destroy);
    wl_list_remove(&popup->commit.link);
    wl_list_remove(&popup->destroy.link);
    free(popup);
}

void xdg_shell_handle_new_popup(struct wl_listener *listener, void *data)
{
    struct wlr_xdg_popup *xdg_popup = data;

    struct pulse_popup *popup = calloc(1, sizeof(*popup));
    if (!popup) {
        wlr_log(WLR_ERROR, "Out of memory allocating pulse_popup");
        return;
    }
    popup->xdg_popup = xdg_popup;

    /* Place the popup under its parent in the scene graph */
    struct wlr_xdg_surface *parent_surface =
        wlr_xdg_surface_try_from_wlr_surface(xdg_popup->parent);
    struct wlr_scene_tree *parent_tree =
        parent_surface ? parent_surface->data : NULL;

    popup->scene_tree = wlr_scene_xdg_surface_create(
        parent_tree ? parent_tree : &((struct pulse_server *)(
            wl_container_of(listener, (struct pulse_server *){NULL},
                            new_xdg_popup)))->scene->tree,
        xdg_popup->base);

    popup->commit.notify = popup_handle_commit;
    wl_signal_add(&xdg_popup->base->surface->events.commit, &popup->commit);

    popup->destroy.notify = popup_handle_destroy;
    wl_signal_add(&xdg_popup->events.destroy, &popup->destroy);
}
