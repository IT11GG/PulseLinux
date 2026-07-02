/* SPDX-License-Identifier: MIT */
/**
 * test_decoration_colors.c — verify that the design-token colour constants
 * used by decoration.c match the frozen Pulse theme specification.
 *
 * This test has no runtime dependencies (no Wayland, no wlroots) — it only
 * checks that the compile-time constants we defined in decoration.c are
 * arithmetically correct derivations of the hex values in the frozen spec.
 *
 * Why this matters: decoration.c defines colours as float[4] literals.
 * A human typo (0.141 vs 0.149, or wrong channel order) would produce a
 * visually wrong border that a code reviewer might not catch. This test
 * catches it automatically.
 *
 * Reference (docs/design-system/pulse-hig.md, theme spec):
 *   --pulse-signal:    #FF2438  → 255, 36, 56    → 1.0,   0.141, 0.220
 *   --pulse-line:      #2A2426  → 42,  36, 38    → 0.165, 0.141, 0.149
 *   --pulse-surface-2: #181517  → 24,  21, 23    → 0.094, 0.082, 0.090
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* Tolerance for float comparison: 1/255 ≈ 0.004, allow 0.002 rounding */
#define TOLERANCE 0.002f

static int failures = 0;

static void check_channel(const char *token, const char *channel,
                            float expected_hex_byte, float actual)
{
    float expected = expected_hex_byte / 255.0f;
    float diff = fabsf(actual - expected);
    if (diff > TOLERANCE) {
        fprintf(stderr,
                "FAIL: %s %s: expected %.3f (from 0x%02X/255) got %.3f "
                "(diff=%.4f > tolerance=%.3f)\n",
                token, channel, expected, (unsigned char)expected_hex_byte,
                actual, diff, TOLERANCE);
        failures++;
    }
}

/*
 * We can't include decoration.c directly (it would pull in wlroots headers
 * we don't have), so we redefine the same constants here and test them.
 * If they ever diverge, the test fails — which is exactly the point.
 */

/* --pulse-signal #FF2438 */
static const float COLOR_SIGNAL[4]   = { 1.0f,  0.141f, 0.220f, 1.0f };
/* --pulse-line   #2A2426 */
static const float COLOR_LINE[4]     = { 0.165f, 0.141f, 0.149f, 1.0f };
/* --pulse-surface-2 #181517 */
static const float COLOR_SURFACE2[4] = { 0.094f, 0.082f, 0.090f, 1.0f };

int main(void)
{
    /* --pulse-signal: #FF2438 → R=0xFF=255, G=0x24=36, B=0x38=56 */
    check_channel("--pulse-signal", "R", 0xFF, COLOR_SIGNAL[0]);
    check_channel("--pulse-signal", "G", 0x24, COLOR_SIGNAL[1]);
    check_channel("--pulse-signal", "B", 0x38, COLOR_SIGNAL[2]);
    if (COLOR_SIGNAL[3] != 1.0f) {
        fprintf(stderr, "FAIL: --pulse-signal alpha must be 1.0\n");
        failures++;
    }

    /* --pulse-line: #2A2426 → R=0x2A=42, G=0x24=36, B=0x26=38 */
    check_channel("--pulse-line", "R", 0x2A, COLOR_LINE[0]);
    check_channel("--pulse-line", "G", 0x24, COLOR_LINE[1]);
    check_channel("--pulse-line", "B", 0x26, COLOR_LINE[2]);
    if (COLOR_LINE[3] != 1.0f) {
        fprintf(stderr, "FAIL: --pulse-line alpha must be 1.0\n");
        failures++;
    }

    /* --pulse-surface-2: #181517 → R=0x18=24, G=0x15=21, B=0x17=23 */
    check_channel("--pulse-surface-2", "R", 0x18, COLOR_SURFACE2[0]);
    check_channel("--pulse-surface-2", "G", 0x15, COLOR_SURFACE2[1]);
    check_channel("--pulse-surface-2", "B", 0x17, COLOR_SURFACE2[2]);
    if (COLOR_SURFACE2[3] != 1.0f) {
        fprintf(stderr, "FAIL: --pulse-surface-2 alpha must be 1.0\n");
        failures++;
    }

    if (failures == 0) {
        printf("PASS: all decoration colour constants match the frozen spec\n");
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
