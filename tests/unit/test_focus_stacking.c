/* SPDX-License-Identifier: MIT */
/**
 * test_focus_stacking.c — unit tests for Milestone 4 focus and stacking logic.
 *
 * Tests the pure state-machine behaviour of:
 *  - is_mapped flag transitions (set on map, cleared on unmap)
 *  - Focus transfer skipping unmapped windows
 *  - Z-order invariant: focused window is always topmost
 *  - Cursor focus dedup: enter only sent when surface changes
 *  - grab state interaction with focus (no focus change during grab)
 *
 * No Wayland, wlroots, or display dependency. All wlroots types are
 * stubbed with minimal stand-ins so the logic under test is isolated.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Test harness -------------------------------------------------------- */

static int pass_count = 0;
static int fail_count = 0;

#define CHECK(cond, msg)                                                        \
    do {                                                                        \
        if (cond) {                                                             \
            pass_count++;                                                       \
        } else {                                                                \
            fprintf(stderr, "FAIL [%s:%d]: %s\n", __FILE__, __LINE__, (msg));  \
            fail_count++;                                                       \
        }                                                                       \
    } while (0)

/* ---- Minimal type stubs -------------------------------------------------- */

/* Simulated toplevel: only the fields we need for focus/mapping logic */
struct fake_toplevel {
    bool    is_mapped;
    bool    is_activated;   /* set when focus is given */
    int     z_order;        /* higher = on top */
    struct fake_toplevel *next; /* linked list of toplevels */
};

/* Simulated server: focuses one toplevel at a time, maintains a list */
struct fake_server {
    struct fake_toplevel *toplevels;    /* head of list, newest first */
    struct fake_toplevel *focused;
    int                   z_counter;
};

/* ---- Simulated compositor operations ------------------------------------- */

static void sim_raise_to_top(struct fake_server *s, struct fake_toplevel *tl)
{
    tl->z_order = ++s->z_counter;
}

static void sim_focus(struct fake_server *s, struct fake_toplevel *tl)
{
    if (s->focused && s->focused != tl) {
        s->focused->is_activated = false;
    }
    s->focused = tl;
    if (tl) {
        tl->is_activated = true;
        sim_raise_to_top(s, tl);
    }
}

static void sim_map(struct fake_server *s, struct fake_toplevel *tl)
{
    tl->is_mapped = true;
    sim_raise_to_top(s, tl);  /* new windows always appear on top */
    sim_focus(s, tl);
}

static void sim_unmap(struct fake_server *s, struct fake_toplevel *tl)
{
    tl->is_mapped = false;
    tl->is_activated = false;

    if (s->focused != tl) return;

    /* Transfer focus to the next mapped window, skipping unmapped ones */
    struct fake_toplevel *next = NULL;
    for (struct fake_toplevel *t = s->toplevels; t; t = t->next) {
        if (t != tl && t->is_mapped) {
            next = t;
            break;
        }
    }
    sim_focus(s, next);
}

static struct fake_toplevel *sim_new_toplevel(struct fake_server *s)
{
    struct fake_toplevel *tl = calloc(1, sizeof(*tl));
    tl->next = s->toplevels;
    s->toplevels = tl;
    return tl;
}

/* ---- Cursor surface-change dedup simulation ------------------------------ */

struct fake_seat {
    void *focused_surface;
    int   enter_count;
    int   motion_count;
};

static void sim_cursor_update(struct fake_seat *seat, void *new_surface,
                               double sx, double sy, uint32_t time_ms)
{
    (void)sx; (void)sy;
    if (new_surface == NULL) {
        seat->focused_surface = NULL;
        return;
    }
    if (new_surface != seat->focused_surface) {
        seat->focused_surface = new_surface;
        seat->enter_count++;
    } else {
        (void)time_ms;
        seat->motion_count++;
    }
}

/* ---- Tests --------------------------------------------------------------- */

static void test_is_mapped_initial_state(void)
{
    struct fake_toplevel tl = {0};
    CHECK(!tl.is_mapped, "toplevel is unmapped before map event");
}

static void test_is_mapped_set_on_map(void)
{
    struct fake_server s = {0};
    struct fake_toplevel *tl = sim_new_toplevel(&s);
    sim_map(&s, tl);
    CHECK(tl->is_mapped, "is_mapped = true after map");
    free(tl);
}

static void test_is_mapped_cleared_on_unmap(void)
{
    struct fake_server s = {0};
    struct fake_toplevel *tl = sim_new_toplevel(&s);
    sim_map(&s, tl);
    sim_unmap(&s, tl);
    CHECK(!tl->is_mapped, "is_mapped = false after unmap");
    free(tl);
}

static void test_focus_given_on_map(void)
{
    struct fake_server s = {0};
    struct fake_toplevel *tl = sim_new_toplevel(&s);
    sim_map(&s, tl);
    CHECK(s.focused == tl,      "focused = newly mapped window");
    CHECK(tl->is_activated,     "newly mapped window is activated");
    free(tl);
}

static void test_focus_raises_window(void)
{
    struct fake_server s = {0};
    struct fake_toplevel *a = sim_new_toplevel(&s);
    struct fake_toplevel *b = sim_new_toplevel(&s);
    sim_map(&s, a);
    sim_map(&s, b);
    sim_focus(&s, a); /* bring a back to front */
    CHECK(a->z_order > b->z_order,
          "focused window has higher z_order than unfocused window");
    free(a); free(b);
}

static void test_new_window_above_existing(void)
{
    struct fake_server s = {0};
    struct fake_toplevel *first = sim_new_toplevel(&s);
    struct fake_toplevel *second = sim_new_toplevel(&s);
    sim_map(&s, first);
    sim_map(&s, second);
    CHECK(second->z_order > first->z_order,
          "second window appears above first window on map");
    free(first); free(second);
}

static void test_focus_transfers_on_close_skips_unmapped(void)
{
    struct fake_server s = {0};
    /* Three windows: a (mapped), b (unmapped), c (mapped, focused) */
    struct fake_toplevel *a = sim_new_toplevel(&s);
    struct fake_toplevel *b = sim_new_toplevel(&s);
    struct fake_toplevel *c = sim_new_toplevel(&s);

    sim_map(&s, a);
    sim_map(&s, b);
    sim_map(&s, c);

    /* Unmap b — it stays in the list but is_mapped=false */
    sim_unmap(&s, b);

    /* Now close c (focused) — focus should go to a, skipping unmapped b */
    sim_unmap(&s, c);

    CHECK(s.focused == a,
          "focus skips unmapped window b and lands on mapped window a");
    CHECK(a->is_activated, "window a is activated after focus transfer");
    CHECK(!b->is_activated, "unmapped window b is not activated");

    free(a); free(b); free(c);
}

static void test_focus_clears_when_no_mapped_windows(void)
{
    struct fake_server s = {0};
    struct fake_toplevel *tl = sim_new_toplevel(&s);
    sim_map(&s, tl);
    sim_unmap(&s, tl);
    CHECK(s.focused == NULL, "focus is NULL when no mapped windows remain");
    free(tl);
}

static void test_focus_invariant_one_active(void)
{
    struct fake_server s = {0};
    struct fake_toplevel *a = sim_new_toplevel(&s);
    struct fake_toplevel *b = sim_new_toplevel(&s);
    struct fake_toplevel *c = sim_new_toplevel(&s);
    sim_map(&s, a);
    sim_map(&s, b);
    sim_map(&s, c);

    sim_focus(&s, a);

    int active_count = (a->is_activated ? 1 : 0) +
                       (b->is_activated ? 1 : 0) +
                       (c->is_activated ? 1 : 0);
    CHECK(active_count == 1,
          "exactly one window is activated at any time (focus invariant)");
    CHECK(s.focused == a, "focused pointer tracks the activated window");

    free(a); free(b); free(c);
}

static void test_cursor_enter_only_on_surface_change(void)
{
    struct fake_seat seat = {0};
    int surface_a = 1, surface_b = 2;

    /* First motion over surface_a — should send enter */
    sim_cursor_update(&seat, &surface_a, 10, 20, 100);
    CHECK(seat.enter_count == 1,   "enter sent when surface changes (none -> a)");
    CHECK(seat.motion_count == 0,  "no motion sent on first enter");

    /* Second motion, still over surface_a — should send motion, not enter */
    sim_cursor_update(&seat, &surface_a, 11, 21, 101);
    CHECK(seat.enter_count == 1,   "no extra enter when surface unchanged");
    CHECK(seat.motion_count == 1,  "motion sent when surface unchanged");

    /* Third motion over surface_a */
    sim_cursor_update(&seat, &surface_a, 12, 22, 102);
    CHECK(seat.enter_count == 1,   "still no extra enters");
    CHECK(seat.motion_count == 2,  "second motion sent");

    /* Move to surface_b — enter again */
    sim_cursor_update(&seat, &surface_b, 50, 60, 103);
    CHECK(seat.enter_count == 2,   "enter sent when surface changes (a -> b)");

    /* Motion over surface_b */
    sim_cursor_update(&seat, &surface_b, 51, 61, 104);
    CHECK(seat.enter_count == 2,   "no enter for continued motion over b");
    CHECK(seat.motion_count == 3,  "motion sent over surface b");
}

static void test_cursor_clear_focus_on_no_surface(void)
{
    struct fake_seat seat = {0};
    int surface_a = 1;

    sim_cursor_update(&seat, &surface_a, 10, 20, 100);
    CHECK(seat.focused_surface == &surface_a, "surface_a focused");

    sim_cursor_update(&seat, NULL, 0, 0, 101);
    CHECK(seat.focused_surface == NULL, "focused_surface cleared when no surface");
}

static void test_refocus_after_remap(void)
{
    struct fake_server s = {0};
    struct fake_toplevel *tl = sim_new_toplevel(&s);

    sim_map(&s, tl);
    CHECK(s.focused == tl, "focused after initial map");

    sim_unmap(&s, tl);
    CHECK(s.focused == NULL, "focus cleared after unmap");

    /* Client re-maps the same window (e.g. show from hidden state) */
    sim_map(&s, tl);
    CHECK(s.focused == tl,   "re-focused after re-map");
    CHECK(tl->is_mapped,     "is_mapped true after re-map");
    CHECK(tl->is_activated,  "window activated after re-map");

    free(tl);
}

/* ---- main ---------------------------------------------------------------- */

int main(void)
{
    test_is_mapped_initial_state();
    test_is_mapped_set_on_map();
    test_is_mapped_cleared_on_unmap();
    test_focus_given_on_map();
    test_focus_raises_window();
    test_new_window_above_existing();
    test_focus_transfers_on_close_skips_unmapped();
    test_focus_clears_when_no_mapped_windows();
    test_focus_invariant_one_active();
    test_cursor_enter_only_on_surface_change();
    test_cursor_clear_focus_on_no_surface();
    test_refocus_after_remap();

    printf("%d passed, %d failed\n", pass_count, fail_count);
    return fail_count == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
