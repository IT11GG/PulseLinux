/* SPDX-License-Identifier: MIT */
/**
 * main.c — PulseDE compositor entry point.
 *
 * Responsibilities:
 *  1. Parse command-line arguments (--backend, --log-level).
 *  2. Initialise the wlroots log handler.
 *  3. Allocate and initialise the pulse_server.
 *  4. Run the Wayland event loop.
 *  5. Tear down and exit cleanly.
 *
 * Everything compositor-specific lives in server.c and the subsystem files.
 * main.c deliberately contains no business logic.
 */

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <wlr/util/log.h>

#include "pulse-server.h"
#include "pulse-version.h"

/* Populated by the --backend argument; NULL means "let wlroots auto-detect". */
static const char *forced_backend = NULL;

/* Signal flag — set by the SIGTERM/SIGINT handler so the event loop exits. */
static volatile sig_atomic_t should_exit = 0;

static void signal_handler(int sig)
{
    (void)sig;
    should_exit = 1;
}

static void usage(const char *progname)
{
    fprintf(stderr,
            "Usage: %s [OPTIONS]\n"
            "\n"
            "PulseDE Wayland compositor v%s\n"
            "\n"
            "Options:\n"
            "  --backend=TYPE   Force a specific wlroots backend.\n"
            "                   TYPE: drm, wayland, x11, headless\n"
            "                   Default: auto-detected.\n"
            "  --log-level=LVL  Log verbosity: silent, error, info, debug.\n"
            "                   Default: info.\n"
            "  --help           Print this help and exit.\n"
            "  --version        Print version and exit.\n",
            progname, PULSE_VERSION);
}

int main(int argc, char *argv[])
{
    /* ---- Signal handling ----------------------------------------------- */
    struct sigaction sa = {.sa_handler = signal_handler};
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    /* ---- Argument parsing ---------------------------------------------- */
    enum wlr_log_importance log_level = WLR_INFO;

    static const struct option long_opts[] = {
        {"backend",   required_argument, NULL, 'b'},
        {"log-level", required_argument, NULL, 'l'},
        {"help",      no_argument,       NULL, 'h'},
        {"version",   no_argument,       NULL, 'V'},
        {NULL, 0, NULL, 0},
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "b:l:hV", long_opts, NULL)) != -1) {
        switch (opt) {
        case 'b':
            forced_backend = optarg;
            break;
        case 'l':
            if (strcmp(optarg, "silent") == 0) {
                log_level = WLR_SILENT;
            } else if (strcmp(optarg, "error") == 0) {
                log_level = WLR_ERROR;
            } else if (strcmp(optarg, "info") == 0) {
                log_level = WLR_INFO;
            } else if (strcmp(optarg, "debug") == 0) {
                log_level = WLR_DEBUG;
            } else {
                fprintf(stderr, "Unknown log level: %s\n", optarg);
                usage(argv[0]);
                return EXIT_FAILURE;
            }
            break;
        case 'h':
            usage(argv[0]);
            return EXIT_SUCCESS;
        case 'V':
            printf("PulseDE %s\n", PULSE_VERSION_FULL);
            return EXIT_SUCCESS;
        default:
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    /* ---- wlroots logging ----------------------------------------------- */
    wlr_log_init(log_level, NULL);
    wlr_log(WLR_INFO, "PulseDE %s starting", PULSE_VERSION_FULL);

    /* ---- Server initialisation ----------------------------------------- */
    struct pulse_server server = {0};

    /* Pass the forced backend through the environment variable that wlroots
     * backends check, so server_init() doesn't need to know about it. */
    if (forced_backend != NULL) {
        setenv("WLR_BACKENDS", forced_backend, 1);
    }

    if (!server_init(&server)) {
        wlr_log(WLR_ERROR, "Failed to initialise PulseDE — see errors above");
        return EXIT_FAILURE;
    }

    /* ---- Event loop ---------------------------------------------------- */
    bool ok = server_run(&server);

    /* ---- Teardown ------------------------------------------------------ */
    server_finish(&server);

    wlr_log(WLR_INFO, "PulseDE exited %s", ok ? "cleanly" : "with error");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
