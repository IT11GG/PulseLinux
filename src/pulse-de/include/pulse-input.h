/* SPDX-License-Identifier: MIT */
/**
 * pulse-input.h — keyboard and pointer input handling.
 */

#pragma once

#include <wayland-server-core.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>

struct pulse_server;

/**
 * pulse_keyboard - per-keyboard state
 */
struct pulse_keyboard {
    struct wl_list       link;   /* server->keyboards */
    struct pulse_server *server;
    struct wlr_keyboard *wlr_keyboard;

    struct wl_listener   modifiers;
    struct wl_listener   key;
    struct wl_listener   destroy;
};

/**
 * input_handle_new_input() - registered with backend.events.new_input.
 *
 * Dispatches to input_new_keyboard() or input_new_pointer() based on
 * device type.
 */
void input_handle_new_input(struct wl_listener *listener, void *data);

/* Internal — called from input_handle_new_input() */
void input_new_keyboard(struct pulse_server *server, struct wlr_input_device *device);
void input_new_pointer(struct pulse_server *server, struct wlr_input_device *device);
