/* SPDX-License-Identifier: MIT */
/**
 * clock.c — clock string formatting and Cairo text rendering for Pulse Panel.
 *
 * Keeps the clock logic isolated so it can be unit-tested without a
 * Wayland connection. clock_update() formats the current time into
 * state->clock_str ("HH:MM"). clock_draw() renders it onto a Cairo
 * surface using the Pulse typography tokens.
 *
 * Milestone 5 scope: HH:MM only, right-aligned, secondary text colour.
 * Future: seconds, date, calendar popup.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <cairo/cairo.h>

#include "panel.h"

/* ---------------------------------------------------------------------------
 * clock_update — write the current local time into state->clock_str.
 *
 * Format: "HH:MM" (24-hour, zero-padded, 5 chars + NUL).
 * Called once per second from the panel's event loop.
 * ------------------------------------------------------------------------- */
void clock_update(struct panel_state *state)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    snprintf(state->clock_str, sizeof(state->clock_str),
             "%02d:%02d", tm->tm_hour, tm->tm_min);
}

/* ---------------------------------------------------------------------------
 * clock_draw — render the clock string onto an existing Cairo context.
 *
 * The text is right-aligned with PANEL_MARGIN_PX padding from the right
 * edge and vertically centred within the panel height.
 *
 * Typography:
 *   Font family:  PANEL_CLOCK_FONT ("Inter Tight"), falling back to
 *                 the system sans-serif if not installed.
 *   Size:         PANEL_CLOCK_SIZE_PX (13px device pixels)
 *   Weight:       normal — the clock is secondary information, per the
 *                 design spec which reserves bold for primary labels.
 *   Color:        --pulse-text-secondary (#9C8E91)
 * ------------------------------------------------------------------------- */
void clock_draw(cairo_t *cr, const char *time_str,
                int surface_width, int surface_height)
{
    if (!time_str || time_str[0] == '\0') {
        return;
    }

    /* Set up font */
    cairo_select_font_face(cr, PANEL_CLOCK_FONT,
                            CAIRO_FONT_SLANT_NORMAL,
                            CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, PANEL_CLOCK_SIZE_PX);

    /* Measure text to right-align it */
    cairo_text_extents_t ext;
    cairo_text_extents(cr, time_str, &ext);

    /* Right edge: surface_width - PANEL_MARGIN_PX - 2px extra breathing room */
    double x = (double)surface_width - ext.width - ext.x_bearing
               - PANEL_MARGIN_PX - 2.0;

    /* Vertical centre */
    double y = ((double)surface_height - ext.height) / 2.0 - ext.y_bearing;

    /* Draw with --pulse-text-secondary colour */
    cairo_set_source_rgb(cr, PANEL_CLOCK_R, PANEL_CLOCK_G, PANEL_CLOCK_B);
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, time_str);
}
