/* SPDX-License-Identifier: MIT */
/**
 * main.c — Pulse Panel entry point.
 *
 * pulse-panel is a standalone Wayland client; it must be run after
 * pulse-de is already listening on a Wayland socket. It reads
 * WAYLAND_DISPLAY from the environment (set automatically by pulse-de
 * when it starts) and exits with a clear error if the compositor does
 * not advertise wlr-layer-shell-unstable-v1.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "panel.h"
#include "pulse-version.h"

static void usage(const char *progname)
{
    fprintf(stderr,
            "Usage: %s [OPTIONS]\n"
            "\n"
            "Pulse Panel v%s — the PulseLinux desktop panel.\n"
            "\n"
            "Options:\n"
            "  --help     Print this help and exit.\n"
            "  --version  Print version and exit.\n"
            "\n"
            "Environment:\n"
            "  WAYLAND_DISPLAY  Wayland socket to connect to (set by pulse-de).\n",
            progname, PULSE_VERSION);
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        }
        if (strcmp(argv[i], "--version") == 0) {
            printf("pulse-panel %s\n", PULSE_VERSION_FULL);
            return EXIT_SUCCESS;
        }
        fprintf(stderr, "pulse-panel: unknown argument: %s\n", argv[i]);
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    return panel_run();
}
