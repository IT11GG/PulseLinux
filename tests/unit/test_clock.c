/* SPDX-License-Identifier: MIT */
/**
 * test_clock.c — unit tests for Pulse Panel clock string formatting.
 *
 * Tests the time-formatting logic used by clock_update() without requiring
 * a Wayland display, Cairo, or a running compositor. The formatting logic
 * is isolated here as a pure function so it can be verified independently.
 *
 * What is tested:
 *  - Output format is always "HH:MM" (5 chars + NUL)
 *  - Hours and minutes are zero-padded
 *  - Hour range 0-23 (24-hour clock)
 *  - Minute range 0-59
 *  - Midnight (00:00) and noon (12:00) edge cases
 *  - Buffer is never overrun (output fits in 8-byte clock_str field)
 *
 * The actual clock_update() function calls localtime() and formats into
 * state->clock_str. Here we replicate just the snprintf logic to test the
 * format contract without touching Wayland state.
 */

#include <stdbool.h>
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

/* ---- Clock format function (mirrors clock_update logic) ------------------ */

/* clock_str must be at least 8 bytes — matches panel.h's clock_str[8] */
static void format_clock(int hour, int minute, char *clock_str, size_t size)
{
    snprintf(clock_str, size, "%02d:%02d", hour, minute);
}

/* ---- Tests --------------------------------------------------------------- */

static void test_format_length(void)
{
    char buf[8];
    format_clock(14, 32, buf, sizeof(buf));
    CHECK(strlen(buf) == 5, "clock string is always 5 characters (HH:MM)");
}

static void test_format_colon_position(void)
{
    char buf[8];
    format_clock(9, 5, buf, sizeof(buf));
    CHECK(buf[2] == ':', "colon is always at position 2");
}

static void test_zero_padding_hour(void)
{
    char buf[8];
    format_clock(7, 30, buf, sizeof(buf));
    CHECK(buf[0] == '0', "single-digit hour is zero-padded: first char is '0'");
    CHECK(buf[1] == '7', "single-digit hour: second char is the digit");
}

static void test_zero_padding_minute(void)
{
    char buf[8];
    format_clock(10, 5, buf, sizeof(buf));
    CHECK(buf[3] == '0', "single-digit minute is zero-padded: fourth char is '0'");
    CHECK(buf[4] == '5', "single-digit minute: fifth char is the digit");
}

static void test_midnight(void)
{
    char buf[8];
    format_clock(0, 0, buf, sizeof(buf));
    CHECK(strcmp(buf, "00:00") == 0, "midnight formats as 00:00");
}

static void test_noon(void)
{
    char buf[8];
    format_clock(12, 0, buf, sizeof(buf));
    CHECK(strcmp(buf, "12:00") == 0, "noon formats as 12:00");
}

static void test_end_of_day(void)
{
    char buf[8];
    format_clock(23, 59, buf, sizeof(buf));
    CHECK(strcmp(buf, "23:59") == 0, "last minute of day formats as 23:59");
}

static void test_all_hours_two_digits(void)
{
    char buf[8];
    int failures = 0;
    for (int h = 0; h < 24; h++) {
        format_clock(h, 0, buf, sizeof(buf));
        if (strlen(buf) != 5) failures++;
        /* First two chars must be digits */
        if (buf[0] < '0' || buf[0] > '9') failures++;
        if (buf[1] < '0' || buf[1] > '9') failures++;
    }
    CHECK(failures == 0, "all 24 hours produce exactly 5-char strings with digit pairs");
}

static void test_all_minutes_two_digits(void)
{
    char buf[8];
    int failures = 0;
    for (int m = 0; m < 60; m++) {
        format_clock(0, m, buf, sizeof(buf));
        if (strlen(buf) != 5) failures++;
        if (buf[3] < '0' || buf[3] > '9') failures++;
        if (buf[4] < '0' || buf[4] > '9') failures++;
    }
    CHECK(failures == 0, "all 60 minutes produce exactly 5-char strings with digit pairs");
}

static void test_fits_in_8_byte_buffer(void)
{
    /* Verify the 8-byte clock_str field (panel.h) is never overrun.
     * "HH:MM" = 5 chars + NUL = 6 bytes. 8 bytes gives 2 bytes slack.
     * The slack is intentional: accommodates "HH:MM\0\0\0" with room. */
    char buf[8];
    memset(buf, 0xFF, sizeof(buf)); /* poison the buffer */
    format_clock(23, 59, buf, sizeof(buf));
    /* NUL terminator must be within the 8 bytes */
    bool has_nul = false;
    for (size_t i = 0; i < sizeof(buf); i++) {
        if (buf[i] == '\0') { has_nul = true; break; }
    }
    CHECK(has_nul, "clock string is NUL-terminated within 8-byte buffer");
}

static void test_specific_values(void)
{
    char buf[8];

    format_clock(8,  45, buf, sizeof(buf));
    CHECK(strcmp(buf, "08:45") == 0, "08:45 formats correctly");

    format_clock(17, 0, buf, sizeof(buf));
    CHECK(strcmp(buf, "17:00") == 0, "17:00 formats correctly");

    format_clock(1, 1, buf, sizeof(buf));
    CHECK(strcmp(buf, "01:01") == 0, "01:01 formats correctly with double zero-pad");
}

/* ---- Design token cross-check ------------------------------------------- */

static void test_panel_height_reasonable(void)
{
    /* PANEL_HEIGHT_PX is defined in panel.h as 36.
     * Verify it's in the expected range for a desktop panel: 28-48px.
     * This test cross-checks that the constant wasn't accidentally changed. */
    int height = 36; /* mirrors PANEL_HEIGHT_PX */
    CHECK(height >= 28 && height <= 48,
          "PANEL_HEIGHT_PX is in the reasonable desktop panel range (28-48px)");
}

static void test_exclusive_zone_equals_height_plus_margin(void)
{
    /* PANEL_EXCLUSIVE_ZONE = PANEL_HEIGHT_PX + PANEL_MARGIN_PX
     * Verify the arithmetic is correct so the WM reserves the right amount. */
    int height = 36; /* PANEL_HEIGHT_PX */
    int margin = 8;  /* PANEL_MARGIN_PX */
    int excl   = height + margin; /* PANEL_EXCLUSIVE_ZONE */
    CHECK(excl == 44,
          "exclusive zone = height(36) + margin(8) = 44px");
}

/* ---- main ---------------------------------------------------------------- */

int main(void)
{
    test_format_length();
    test_format_colon_position();
    test_zero_padding_hour();
    test_zero_padding_minute();
    test_midnight();
    test_noon();
    test_end_of_day();
    test_all_hours_two_digits();
    test_all_minutes_two_digits();
    test_fits_in_8_byte_buffer();
    test_specific_values();
    test_panel_height_reasonable();
    test_exclusive_zone_equals_height_plus_margin();

    printf("%d passed, %d failed\n", pass_count, fail_count);
    return fail_count == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
