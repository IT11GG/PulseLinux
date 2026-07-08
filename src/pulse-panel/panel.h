/* SPDX-License-Identifier: MIT */
/**
 * panel.h — Pulse Panel internal state and function declarations.
 *
 * The Pulse Panel is a standalone Wayland client. It connects to PulseDE's
 * Wayland socket, binds the wlr-layer-shell-unstable-v1 protocol, and
 * creates one layer surface per output anchored to the bottom of the screen.
 *
 * Rendering uses shared memory (wl_shm) buffers written with Cairo for
 * text and pixman for solid fills. No GPU dependency — the compositor
 * handles final compositing.
 *
 * Design tokens (frozen from the Pulse design system):
 *   Background:  #050505  —  --pulse-void (panel surface fill)
 *   Text:        #9C8E91  —  --pulse-text-secondary (clock digits)
 *   Height:      36px     —  matches HIG §taskbar floating pill height
 *   Corner radius: 0px    —  M5: no clipping yet, full rectangle
 *   Margins:     8px bottom, 8px sides  — floating inset from screen edge
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include <cairo/cairo.h>
#include <wayland-client.h>

/* Generated protocol headers */
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

/* ---- Design tokens (compile-time, superseded by Theme Engine in M6+) ---- */
#define PANEL_HEIGHT_PX      36
#define PANEL_MARGIN_PX       8
#define PANEL_EXCLUSIVE_ZONE (PANEL_HEIGHT_PX + PANEL_MARGIN_PX)

/* --pulse-void #050505 as 0xAARRGGBB (wl_shm ARGB8888 format) */
#define PANEL_COLOR_BG      0xFF050505u

/* --pulse-text-secondary #9C8E91 */
#define PANEL_CLOCK_R       (0x9C / 255.0)
#define PANEL_CLOCK_G       (0x8E / 255.0)
#define PANEL_CLOCK_B       (0x91 / 255.0)

/* Clock font — Inter Tight is the display font per the design spec.
 * Falls back through the system font stack if not installed. */
#define PANEL_CLOCK_FONT    "Inter Tight"
#define PANEL_CLOCK_SIZE_PX  13.0

/* ---- Shared memory buffer ------------------------------------------------ */

struct panel_buffer {
    struct wl_buffer *wl_buffer;
    void             *data;       /* mmap'd shm data */
    int               width;
    int               height;
    int               stride;     /* bytes per row = width * 4 */
    size_t            size;       /* total bytes */
    bool              in_use;     /* true while compositor holds this buffer */
};

/* ---- Per-output panel surface -------------------------------------------- */

struct panel_output {
    struct wl_list              link;     /* panel_state.outputs */
    struct panel_state         *state;

    struct wl_output           *wl_output;
    int                         output_width;   /* pixels */
    int                         output_height;

    struct zwlr_layer_surface_v1 *layer_surface;
    struct wl_surface            *wl_surface;

    /* Double-buffered rendering */
    struct panel_buffer          buf[2];
    int                          active_buf;  /* index into buf[] */

    bool                         configured;  /* received first configure */
    uint32_t                     configured_w;
    uint32_t                     configured_h;

    struct wl_listener_list      *listeners;
};

/* ---- Global panel state -------------------------------------------------- */

struct panel_state {
    /* Wayland client globals */
    struct wl_display    *display;
    struct wl_registry   *registry;
    struct wl_compositor *compositor;
    struct wl_shm        *shm;
    struct zwlr_layer_shell_v1 *layer_shell;

    /* Outputs */
    struct wl_list        outputs;  /* list of struct panel_output */

    /* Clock state */
    char                  clock_str[8]; /* "HH:MM\0" */
    bool                  running;
};

/* ---- Function declarations ----------------------------------------------- */

/* panel.c */
int  panel_run(void);
void panel_render_output(struct panel_output *out);

/* clock.c */
void clock_update(struct panel_state *state);
void clock_draw(cairo_t *cr, const char *time_str,
                int surface_width, int surface_height);

/* buffer management (panel.c) */
struct panel_buffer *panel_buffer_acquire(struct panel_output *out);
void                 panel_buffer_init(struct panel_buffer *buf,
                                       struct wl_shm *shm,
                                       int width, int height);
void                 panel_buffer_destroy(struct panel_buffer *buf);
