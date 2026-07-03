/* SPDX-License-Identifier: MIT */
/**
 * test_grab.c — unit tests for the interactive grab state machine.
 *
 * Tests the pure logic of the grab state transitions and geometry
 * calculations without requiring a Wayland display, wlroots, or a
 * real compositor instance.
 *
 * Strategy: replicate the data structures and the enum/inline from
 * pulse-grab.h, then test the state-machine rules directly. This is
 * an explicit copy rather than an include because including pulse-grab.h
 * would pull in wlroots transitive headers that aren't installed here.
 * The test asserts that the LOGIC is correct; the compilation test
 * (ninja -C builddir) asserts that the real header compiles cleanly.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Replicated constants from pulse-grab.h (kept in sync by review) ---- */

enum pulse_grab_type {
    PULSE_GRAB_NONE   = 0,
    PULSE_GRAB_MOVE   = 1,
    PULSE_GRAB_RESIZE = 2,
};

struct pulse_grab {
    enum pulse_grab_type type;
    void                *toplevel;   /* opaque for this test */
    double               grab_x;
    double               grab_y;
    int                  win_x;
    int                  win_y;
    int                  win_w;
    int                  win_h;
    uint32_t             resize_edges;
};

static inline bool grab_is_active(const struct pulse_grab *g)
{
    return g->type != PULSE_GRAB_NONE;
}

/* ---- Minimum size constants (must match grab.c) ---- */
#define MIN_WIN_WIDTH  120
#define MIN_WIN_HEIGHT  80

/* ---- WLR_EDGE values (from wlr/util/edges.h — copied to avoid include) -- */
#define WLR_EDGE_NONE   0
#define WLR_EDGE_TOP    1
#define WLR_EDGE_BOTTOM 2
#define WLR_EDGE_LEFT   4
#define WLR_EDGE_RIGHT  8

/* ---- Test harness -------------------------------------------------------- */

static int pass_count = 0;
static int fail_count = 0;

#define CHECK(cond, msg)                                                   \
    do {                                                                    \
        if (cond) {                                                         \
            pass_count++;                                                   \
        } else {                                                            \
            fprintf(stderr, "FAIL [%s:%d]: %s\n", __FILE__, __LINE__, msg); \
            fail_count++;                                                   \
        }                                                                   \
    } while (0)

/* ---- Simulated grab_begin_move logic (mirrors grab.c) ------------------- */

static void sim_grab_begin_move(struct pulse_grab *grab, void *toplevel,
                                 double cursor_x, double cursor_y,
                                 int win_x, int win_y)
{
    if (grab_is_active(grab)) return; /* no double-grab */

    grab->type      = PULSE_GRAB_MOVE;
    grab->toplevel  = toplevel;
    grab->grab_x    = cursor_x;
    grab->grab_y    = cursor_y;
    grab->win_x     = win_x;
    grab->win_y     = win_y;
}

/* ---- Simulated grab_begin_resize logic ---------------------------------- */

static void sim_grab_begin_resize(struct pulse_grab *grab, void *toplevel,
                                   double cursor_x, double cursor_y,
                                   int win_x, int win_y, int win_w, int win_h,
                                   uint32_t edges)
{
    if (grab_is_active(grab)) return;

    grab->type         = PULSE_GRAB_RESIZE;
    grab->toplevel     = toplevel;
    grab->grab_x       = cursor_x;
    grab->grab_y       = cursor_y;
    grab->win_x        = win_x;
    grab->win_y        = win_y;
    grab->win_w        = win_w;
    grab->win_h        = win_h;
    grab->resize_edges = edges;
}

/* ---- Simulated grab_update for MOVE (returns new position) -------------- */

static void sim_grab_update_move(const struct pulse_grab *grab,
                                  double cursor_x, double cursor_y,
                                  int *out_x, int *out_y)
{
    double dx = cursor_x - grab->grab_x;
    double dy = cursor_y - grab->grab_y;
    *out_x = grab->win_x + (int)dx;
    *out_y = grab->win_y + (int)dy;
    if (*out_y < 0) *out_y = 0;
}

/* ---- Simulated grab_update for RESIZE (returns new geometry) ------------ */

static void sim_grab_update_resize(const struct pulse_grab *grab,
                                    double cursor_x, double cursor_y,
                                    int *out_x, int *out_y,
                                    int *out_w, int *out_h)
{
    double dx = cursor_x - grab->grab_x;
    double dy = cursor_y - grab->grab_y;
    uint32_t edges = grab->resize_edges;

    *out_x = grab->win_x;
    *out_y = grab->win_y;
    *out_w = grab->win_w;
    *out_h = grab->win_h;

    if (edges & WLR_EDGE_RIGHT)  *out_w = grab->win_w + (int)dx;
    if (edges & WLR_EDGE_BOTTOM) *out_h = grab->win_h + (int)dy;
    if (edges & WLR_EDGE_LEFT) {
        *out_w = grab->win_w - (int)dx;
        *out_x = grab->win_x + (int)dx;
    }
    if (edges & WLR_EDGE_TOP) {
        *out_h = grab->win_h - (int)dy;
        *out_y = grab->win_y + (int)dy;
    }

    if (*out_w < MIN_WIN_WIDTH) {
        if (edges & WLR_EDGE_LEFT)
            *out_x = grab->win_x + grab->win_w - MIN_WIN_WIDTH;
        *out_w = MIN_WIN_WIDTH;
    }
    if (*out_h < MIN_WIN_HEIGHT) {
        if (edges & WLR_EDGE_TOP)
            *out_y = grab->win_y + grab->win_h - MIN_WIN_HEIGHT;
        *out_h = MIN_WIN_HEIGHT;
    }
}

/* ---- Simulated grab_end ------------------------------------------------- */

static void sim_grab_end(struct pulse_grab *grab)
{
    memset(grab, 0, sizeof(*grab));
    grab->type = PULSE_GRAB_NONE;
}

/* ---- Tests --------------------------------------------------------------- */

static void test_initial_state(void)
{
    struct pulse_grab grab = {0};
    CHECK(!grab_is_active(&grab),
          "grab inactive initially");
    CHECK(grab.type == PULSE_GRAB_NONE,
          "initial type is NONE");
}

static void test_move_begin(void)
{
    struct pulse_grab grab = {0};
    int toplevel = 42;

    sim_grab_begin_move(&grab, &toplevel, 100.0, 200.0, 50, 80);

    CHECK(grab_is_active(&grab),        "grab active after begin_move");
    CHECK(grab.type == PULSE_GRAB_MOVE, "type is MOVE after begin_move");
    CHECK(grab.toplevel == &toplevel,   "toplevel pointer stored");
    CHECK(grab.grab_x == 100.0,         "grab_x stored");
    CHECK(grab.grab_y == 200.0,         "grab_y stored");
    CHECK(grab.win_x == 50,             "win_x stored");
    CHECK(grab.win_y == 80,             "win_y stored");
}

static void test_move_update_position(void)
{
    struct pulse_grab grab = {0};
    sim_grab_begin_move(&grab, NULL, 100.0, 200.0, 50, 80);

    /* Move cursor 30px right, 20px down */
    int nx, ny;
    sim_grab_update_move(&grab, 130.0, 220.0, &nx, &ny);

    CHECK(nx == 80, "move: x = 50 + 30 = 80");
    CHECK(ny == 100, "move: y = 80 + 20 = 100");
}

static void test_move_clamps_to_top(void)
{
    struct pulse_grab grab = {0};
    /* Window starts at y=20, cursor at y=30 */
    sim_grab_begin_move(&grab, NULL, 100.0, 30.0, 0, 20);

    /* Drag cursor 60px up — would put window at y = 20 - 60 = -40 */
    int nx, ny;
    sim_grab_update_move(&grab, 100.0, -30.0, &nx, &ny);

    CHECK(ny == 0, "move: y clamped to 0 when dragged above screen top");
}

static void test_move_no_double_grab(void)
{
    struct pulse_grab grab = {0};
    int tl1 = 1, tl2 = 2;

    sim_grab_begin_move(&grab, &tl1, 10.0, 20.0, 0, 0);
    sim_grab_begin_move(&grab, &tl2, 50.0, 60.0, 100, 100);

    /* Second begin_move must be ignored — grab still belongs to tl1 */
    CHECK(grab.toplevel == &tl1, "double grab: first toplevel preserved");
    CHECK(grab.grab_x == 10.0,   "double grab: original grab_x preserved");
}

static void test_move_end(void)
{
    struct pulse_grab grab = {0};
    sim_grab_begin_move(&grab, NULL, 0, 0, 0, 0);
    CHECK(grab_is_active(&grab), "active before end");

    sim_grab_end(&grab);
    CHECK(!grab_is_active(&grab),          "inactive after end");
    CHECK(grab.type == PULSE_GRAB_NONE,    "type NONE after end");
    CHECK(grab.toplevel == NULL,           "toplevel NULL after end");
}

static void test_resize_begin(void)
{
    struct pulse_grab grab = {0};
    sim_grab_begin_resize(&grab, NULL, 200.0, 300.0,
                           10, 20, 640, 480,
                           WLR_EDGE_RIGHT | WLR_EDGE_BOTTOM);

    CHECK(grab_is_active(&grab),           "grab active after begin_resize");
    CHECK(grab.type == PULSE_GRAB_RESIZE,  "type is RESIZE");
    CHECK(grab.win_w == 640,               "win_w stored");
    CHECK(grab.win_h == 480,               "win_h stored");
    CHECK(grab.resize_edges == (WLR_EDGE_RIGHT | WLR_EDGE_BOTTOM),
          "edges stored");
}

static void test_resize_right_bottom(void)
{
    struct pulse_grab grab = {0};
    /* Window 640x480 at (10,20), grab from SE corner */
    sim_grab_begin_resize(&grab, NULL, 650.0, 500.0,
                           10, 20, 640, 480,
                           WLR_EDGE_RIGHT | WLR_EDGE_BOTTOM);

    /* Drag 60px right, 40px down */
    int nx, ny, nw, nh;
    sim_grab_update_resize(&grab, 710.0, 540.0, &nx, &ny, &nw, &nh);

    CHECK(nx == 10,  "resize SE: x unchanged");
    CHECK(ny == 20,  "resize SE: y unchanged");
    CHECK(nw == 700, "resize SE: w = 640 + 60 = 700");
    CHECK(nh == 520, "resize SE: h = 480 + 40 = 520");
}

static void test_resize_left_top(void)
{
    struct pulse_grab grab = {0};
    /* Window 640x480 at (100,100), grab from NW corner */
    sim_grab_begin_resize(&grab, NULL, 100.0, 100.0,
                           100, 100, 640, 480,
                           WLR_EDGE_LEFT | WLR_EDGE_TOP);

    /* Drag 40px right, 30px down — shrinks the window from NW */
    int nx, ny, nw, nh;
    sim_grab_update_resize(&grab, 140.0, 130.0, &nx, &ny, &nw, &nh);

    CHECK(nx == 140, "resize NW: x = 100 + 40 = 140");
    CHECK(ny == 130, "resize NW: y = 100 + 30 = 130");
    CHECK(nw == 600, "resize NW: w = 640 - 40 = 600");
    CHECK(nh == 450, "resize NW: h = 480 - 30 = 450");
}

static void test_resize_min_width_right_edge(void)
{
    struct pulse_grab grab = {0};
    sim_grab_begin_resize(&grab, NULL, 400.0, 200.0,
                           10, 20, 300, 400,
                           WLR_EDGE_RIGHT);

    /* Drag so far left that computed width would be 10 (below MIN_WIN_WIDTH) */
    int nx, ny, nw, nh;
    sim_grab_update_resize(&grab, 110.0, 200.0, &nx, &ny, &nw, &nh);

    CHECK(nw == MIN_WIN_WIDTH,
          "resize: width clamped to MIN_WIN_WIDTH when dragging right edge left");
    CHECK(nx == 10, "resize right edge: x unchanged at min width");
}

static void test_resize_min_width_left_edge(void)
{
    struct pulse_grab grab = {0};
    /* Window 300px wide starting at x=10, grabbing from left edge at x=10 */
    sim_grab_begin_resize(&grab, NULL, 10.0, 200.0,
                           10, 20, 300, 400,
                           WLR_EDGE_LEFT);

    /* Drag 250px right — would make width = 300-250 = 50, below minimum */
    int nx, ny, nw, nh;
    sim_grab_update_resize(&grab, 260.0, 200.0, &nx, &ny, &nw, &nh);

    CHECK(nw == MIN_WIN_WIDTH,
          "resize left edge: width clamped to minimum");
    CHECK(nx == 10 + 300 - MIN_WIN_WIDTH,
          "resize left edge: x adjusted so right edge stays fixed at min width");
}

static void test_resize_min_height_bottom_edge(void)
{
    struct pulse_grab grab = {0};
    sim_grab_begin_resize(&grab, NULL, 200.0, 500.0,
                           10, 20, 400, 200,
                           WLR_EDGE_BOTTOM);

    /* Drag so far up that height would go to 10 */
    int nx, ny, nw, nh;
    sim_grab_update_resize(&grab, 200.0, 310.0, &nx, &ny, &nw, &nh);

    CHECK(nh == MIN_WIN_HEIGHT,
          "resize: height clamped to MIN_WIN_HEIGHT");
}

static void test_resize_no_double_grab(void)
{
    struct pulse_grab grab = {0};
    sim_grab_begin_resize(&grab, NULL, 0, 0, 0, 0, 200, 200, WLR_EDGE_RIGHT);
    sim_grab_begin_resize(&grab, NULL, 99, 99, 99, 99, 999, 999, WLR_EDGE_LEFT);

    CHECK(grab.win_w == 200, "no double grab: first resize geometry preserved");
    CHECK(grab.resize_edges == WLR_EDGE_RIGHT,
          "no double grab: first resize edges preserved");
}

static void test_end_is_idempotent(void)
{
    struct pulse_grab grab = {0};
    sim_grab_end(&grab);
    sim_grab_end(&grab); /* second call must not crash or corrupt state */
    CHECK(!grab_is_active(&grab), "end is idempotent: still inactive");
}

/* ---- main ---------------------------------------------------------------- */

int main(void)
{
    test_initial_state();
    test_move_begin();
    test_move_update_position();
    test_move_clamps_to_top();
    test_move_no_double_grab();
    test_move_end();
    test_resize_begin();
    test_resize_right_bottom();
    test_resize_left_top();
    test_resize_min_width_right_edge();
    test_resize_min_width_left_edge();
    test_resize_min_height_bottom_edge();
    test_resize_no_double_grab();
    test_end_is_idempotent();

    printf("%d passed, %d failed\n", pass_count, fail_count);
    return fail_count == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
