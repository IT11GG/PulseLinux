/* SPDX-License-Identifier: MIT */
/**
 * panel.c — Pulse Panel Wayland client core.
 *
 * Connects to the running PulseDE compositor, binds wlr-layer-shell, and
 * creates one anchored panel surface per output. Renders the clock every
 * second using a 1s timerfd-based poll loop.
 *
 * Buffer management: double-buffered wl_shm (shared memory). Two buffers
 * per output — one held by the compositor while rendering into the other.
 * On the panel's scale this is sufficient; GPU-backed buffers are a future
 * optimisation when animated surfaces are added.
 *
 * Render path:
 *   1. Acquire an idle buffer.
 *   2. Wrap it in a Cairo image surface.
 *   3. Fill with PANEL_COLOR_BG (#050505).
 *   4. Call clock_draw() for the time string.
 *   5. Destroy the Cairo surface (flushes).
 *   6. Attach the wl_buffer to the wl_surface and commit.
 */

/* memfd_create() and MFD_* are GNU extensions hidden behind _GNU_SOURCE
 * (the project sets -D_POSIX_C_SOURCE=200809L globally, which suppresses
 * them). Must be defined before the first system header include. */
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>

#include <cairo/cairo.h>
#include <wayland-client.h>

#include "panel.h"

/* ---------------------------------------------------------------------------
 * shm buffer helpers
 * ------------------------------------------------------------------------- */

static int shm_create_anon_file(size_t size)
{
    int fd = memfd_create("pulse-panel-shm", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd < 0) {
        perror("pulse-panel: memfd_create");
        return -1;
    }
    if (ftruncate(fd, (off_t)size) < 0) {
        perror("pulse-panel: ftruncate");
        close(fd);
        return -1;
    }
    return fd;
}

void panel_buffer_init(struct panel_buffer *buf, struct wl_shm *shm,
                        int width, int height)
{
    buf->width  = width;
    buf->height = height;
    buf->stride = width * 4;
    buf->size   = (size_t)(buf->stride * height);
    buf->in_use = false;
    buf->wl_buffer = NULL;
    buf->data   = NULL;

    int fd = shm_create_anon_file(buf->size);
    if (fd < 0) return;

    buf->data = mmap(NULL, buf->size, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, 0);
    if (buf->data == MAP_FAILED) {
        perror("pulse-panel: mmap");
        close(fd);
        buf->data = NULL;
        return;
    }

    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, (int32_t)buf->size);
    buf->wl_buffer = wl_shm_pool_create_buffer(pool, 0, width, height,
                                                buf->stride,
                                                WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);
}

void panel_buffer_destroy(struct panel_buffer *buf)
{
    if (buf->wl_buffer) {
        wl_buffer_destroy(buf->wl_buffer);
        buf->wl_buffer = NULL;
    }
    if (buf->data) {
        munmap(buf->data, buf->size);
        buf->data = NULL;
    }
}

static void buffer_handle_release(void *data, struct wl_buffer *wl_buffer)
{
    struct panel_buffer *buf = data;
    buf->in_use = false;
}

static const struct wl_buffer_listener buffer_listener = {
    .release = buffer_handle_release,
};

struct panel_buffer *panel_buffer_acquire(struct panel_output *out)
{
    for (int i = 0; i < 2; i++) {
        if (!out->buf[i].in_use && out->buf[i].wl_buffer != NULL) {
            out->buf[i].in_use = true;
            return &out->buf[i];
        }
    }
    return NULL; /* both buffers in use — skip this frame */
}

/* ---------------------------------------------------------------------------
 * Rendering
 * ------------------------------------------------------------------------- */

void panel_render_output(struct panel_output *out)
{
    if (!out->configured) return;

    int w = (int)out->configured_w;
    int h = (int)out->configured_h;

    /* Lazily initialise buffers on first render or resize */
    if (out->buf[0].width != w || out->buf[0].height != h) {
        panel_buffer_destroy(&out->buf[0]);
        panel_buffer_destroy(&out->buf[1]);
        panel_buffer_init(&out->buf[0], out->state->shm, w, h);
        panel_buffer_init(&out->buf[1], out->state->shm, w, h);
        if (out->buf[0].wl_buffer) {
            wl_buffer_add_listener(out->buf[0].wl_buffer,
                                    &buffer_listener, &out->buf[0]);
        }
        if (out->buf[1].wl_buffer) {
            wl_buffer_add_listener(out->buf[1].wl_buffer,
                                    &buffer_listener, &out->buf[1]);
        }
    }

    struct panel_buffer *buf = panel_buffer_acquire(out);
    if (!buf) return;

    /* ---- Cairo render ---- */
    cairo_surface_t *cs = cairo_image_surface_create_for_data(
        (unsigned char *)buf->data,
        CAIRO_FORMAT_ARGB32,
        w, h, buf->stride);
    cairo_t *cr = cairo_create(cs);

    /* Fill background: --pulse-void #050505 */
    cairo_set_source_rgba(cr,
        ((PANEL_COLOR_BG >> 16) & 0xFF) / 255.0,
        ((PANEL_COLOR_BG >>  8) & 0xFF) / 255.0,
        ((PANEL_COLOR_BG      ) & 0xFF) / 255.0,
        1.0);
    cairo_paint(cr);

    /* Draw clock */
    clock_draw(cr, out->state->clock_str, w, h);

    cairo_destroy(cr);
    cairo_surface_flush(cs);
    cairo_surface_destroy(cs);

    /* Attach and commit */
    wl_surface_attach(out->wl_surface, buf->wl_buffer, 0, 0);
    wl_surface_damage_buffer(out->wl_surface, 0, 0, w, h);
    wl_surface_commit(out->wl_surface);
}

/* ---------------------------------------------------------------------------
 * Layer surface listeners
 * ------------------------------------------------------------------------- */

static void layer_surface_handle_configure(
        void *data,
        struct zwlr_layer_surface_v1 *surface,
        uint32_t serial,
        uint32_t width,
        uint32_t height)
{
    struct panel_output *out = data;
    out->configured   = true;
    out->configured_w = width;
    out->configured_h = height;
    zwlr_layer_surface_v1_ack_configure(surface, serial);
    panel_render_output(out);
}

static void layer_surface_handle_closed(void *data,
        struct zwlr_layer_surface_v1 *surface)
{
    struct panel_output *out = data;
    out->state->running = false;
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_handle_configure,
    .closed    = layer_surface_handle_closed,
};

/* ---------------------------------------------------------------------------
 * Output management
 * ------------------------------------------------------------------------- */

static void create_panel_for_output(struct panel_state *state,
                                     struct panel_output *out)
{
    out->wl_surface = wl_compositor_create_surface(state->compositor);

    out->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        state->layer_shell,
        out->wl_surface,
        out->wl_output,
        ZWLR_LAYER_SHELL_V1_LAYER_TOP,
        "pulse-panel");

    /* Anchor to the bottom, stretch full width */
    zwlr_layer_surface_v1_set_anchor(out->layer_surface,
        ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT   |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);

    zwlr_layer_surface_v1_set_size(out->layer_surface, 0, PANEL_HEIGHT_PX);

    /* Exclusive zone: reserve pixels so windows don't go under the panel */
    zwlr_layer_surface_v1_set_exclusive_zone(out->layer_surface,
                                              PANEL_EXCLUSIVE_ZONE);

    /* Bottom margin creates the floating gap */
    zwlr_layer_surface_v1_set_margin(out->layer_surface,
        0, 0, PANEL_MARGIN_PX, 0);

    zwlr_layer_surface_v1_add_listener(out->layer_surface,
                                        &layer_surface_listener, out);

    /* Commit the surface configuration (no buffer yet — wait for configure) */
    wl_surface_commit(out->wl_surface);
}

/* ---------------------------------------------------------------------------
 * Registry listeners
 * ------------------------------------------------------------------------- */

static void output_handle_geometry(void *data, struct wl_output *wl_output,
        int32_t x, int32_t y, int32_t physical_width, int32_t physical_height,
        int32_t subpixel, const char *make, const char *model,
        int32_t transform) { (void)data; (void)wl_output; (void)x; (void)y;
    (void)physical_width; (void)physical_height; (void)subpixel;
    (void)make; (void)model; (void)transform; }

static void output_handle_mode(void *data, struct wl_output *wl_output,
        uint32_t flags, int32_t width, int32_t height, int32_t refresh)
{
    struct panel_output *out = data;
    if (flags & WL_OUTPUT_MODE_CURRENT) {
        out->output_width  = width;
        out->output_height = height;
    }
    (void)wl_output; (void)refresh;
}

static void output_handle_done(void *data, struct wl_output *wl_output)
{
    struct panel_output *out = data;
    (void)wl_output;
    if (!out->layer_surface) {
        create_panel_for_output(out->state, out);
    }
}

static void output_handle_scale(void *data, struct wl_output *wl_output,
        int32_t factor) { (void)data; (void)wl_output; (void)factor; }

static void output_handle_name(void *data, struct wl_output *wl_output,
        const char *name) { (void)data; (void)wl_output; (void)name; }

static void output_handle_description(void *data, struct wl_output *wl_output,
        const char *desc) { (void)data; (void)wl_output; (void)desc; }

static const struct wl_output_listener output_listener = {
    .geometry    = output_handle_geometry,
    .mode        = output_handle_mode,
    .done        = output_handle_done,
    .scale       = output_handle_scale,
    .name        = output_handle_name,
    .description = output_handle_description,
};

static void registry_handle_global(void *data, struct wl_registry *registry,
        uint32_t name, const char *interface, uint32_t version)
{
    struct panel_state *state = data;

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->compositor = wl_registry_bind(registry, name,
                                              &wl_compositor_interface, 4);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        state->shm = wl_registry_bind(registry, name,
                                       &wl_shm_interface, 1);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        state->layer_shell = wl_registry_bind(registry, name,
                                               &zwlr_layer_shell_v1_interface,
                                               (version < 4) ? version : 4);
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        struct panel_output *out = calloc(1, sizeof(*out));
        if (!out) return;
        out->state     = state;
        out->wl_output = wl_registry_bind(registry, name,
                                           &wl_output_interface, 4);
        wl_output_add_listener(out->wl_output, &output_listener, out);
        wl_list_insert(&state->outputs, &out->link);
    }
    (void)version;
}

static void registry_handle_global_remove(void *data,
        struct wl_registry *registry, uint32_t name)
{
    (void)data; (void)registry; (void)name;
}

static const struct wl_registry_listener registry_listener = {
    .global        = registry_handle_global,
    .global_remove = registry_handle_global_remove,
};

/* ---------------------------------------------------------------------------
 * panel_run — main entry point
 * ------------------------------------------------------------------------- */

int panel_run(void)
{
    struct panel_state state = {0};
    wl_list_init(&state.outputs);
    state.running = true;

    state.display = wl_display_connect(NULL);
    if (!state.display) {
        fprintf(stderr, "pulse-panel: failed to connect to Wayland display\n");
        return 1;
    }

    state.registry = wl_display_get_registry(state.display);
    wl_registry_add_listener(state.registry, &registry_listener, &state);
    wl_display_roundtrip(state.display); /* bind globals */
    wl_display_roundtrip(state.display); /* receive output events */

    if (!state.layer_shell) {
        fprintf(stderr, "pulse-panel: compositor does not advertise "
                "wlr-layer-shell-unstable-v1\n"
                "  Is pulse-de running and is layer shell initialised?\n");
        return 1;
    }

    /* Initial clock value before the first timer tick */
    clock_update(&state);

    /* Render all outputs */
    wl_display_roundtrip(state.display);

    /* ---- 1-second timer using timerfd ---- */
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    if (tfd < 0) {
        perror("pulse-panel: timerfd_create");
        return 1;
    }
    struct itimerspec ts = {
        .it_interval = {.tv_sec = 1, .tv_nsec = 0},
        .it_value    = {.tv_sec = 1, .tv_nsec = 0},
    };
    timerfd_settime(tfd, 0, &ts, NULL);

    int wfd = wl_display_get_fd(state.display);

    /* ---- Event loop ---- */
    while (state.running) {
        /* Flush pending Wayland requests */
        while (wl_display_prepare_read(state.display) != 0) {
            wl_display_dispatch_pending(state.display);
        }
        wl_display_flush(state.display);

        struct pollfd fds[2] = {
            {.fd = wfd, .events = POLLIN},
            {.fd = tfd, .events = POLLIN},
        };

        int ret = poll(fds, 2, -1);
        if (ret < 0) {
            if (errno == EINTR) {
                wl_display_cancel_read(state.display);
                continue;
            }
            wl_display_cancel_read(state.display);
            perror("pulse-panel: poll");
            break;
        }

        /* Handle Wayland events */
        if (fds[0].revents & POLLIN) {
            wl_display_read_events(state.display);
            wl_display_dispatch_pending(state.display);
        } else {
            wl_display_cancel_read(state.display);
        }

        /* Handle 1s timer tick — update clock and re-render */
        if (fds[1].revents & POLLIN) {
            uint64_t expirations;
            read(tfd, &expirations, sizeof(expirations));
            clock_update(&state);
            struct panel_output *out;
            wl_list_for_each(out, &state.outputs, link) {
                panel_render_output(out);
            }
            wl_display_flush(state.display);
        }
    }

    /* ---- Cleanup ---- */
    close(tfd);

    struct panel_output *out, *tmp;
    wl_list_for_each_safe(out, tmp, &state.outputs, link) {
        panel_buffer_destroy(&out->buf[0]);
        panel_buffer_destroy(&out->buf[1]);
        if (out->layer_surface)
            zwlr_layer_surface_v1_destroy(out->layer_surface);
        if (out->wl_surface)
            wl_surface_destroy(out->wl_surface);
        if (out->wl_output)
            wl_output_release(out->wl_output);
        wl_list_remove(&out->link);
        free(out);
    }

    if (state.layer_shell) zwlr_layer_shell_v1_destroy(state.layer_shell);
    if (state.shm)         wl_shm_destroy(state.shm);
    if (state.compositor)  wl_compositor_destroy(state.compositor);
    wl_registry_destroy(state.registry);
    wl_display_disconnect(state.display);

    return 0;
}
