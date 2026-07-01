/* SPDX-License-Identifier: MIT */
/**
 * input.c — keyboard and pointer input handling for PulseDE.
 *
 * Keyboard events:
 *  - XKB layout initialisation (defaults: us layout, no options)
 *  - Modifier handling (sent to focused client via wlr_seat)
 *  - Compositor key bindings (Super+L = lock; Super+Q = quit for debug)
 *  - All other keys forwarded to the focused client
 *
 * Pointer events:
 *  - Motion updates → wlr_cursor → pointer focus via hit-testing
 *  - Button events → focus-on-click + wlr_seat notify
 *  - Axis (scroll) events → wlr_seat notify
 */

#include <stdlib.h>
#include <string.h>

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>

#include "pulse-input.h"
#include "pulse-server.h"
#include "pulse-xdg-shell.h"

/* ---------------------------------------------------------------------------
 * Keyboard
 * ------------------------------------------------------------------------- */

/**
 * handle_compositor_keybinding() - intercept compositor-level shortcuts.
 *
 * Returns true if the key was consumed by the compositor (and must NOT be
 * forwarded to the focused client). Returns false if the key should be
 * passed through.
 *
 * Bindings (HIG §11.2 — Super key namespace):
 *   Super+L  : lock screen (not yet wired to Session Manager in M1 — logs)
 *   Super+Q  : terminate compositor (developer shortcut, will be removed)
 */
static bool handle_compositor_keybinding(struct pulse_server *server,
                                          xkb_keysym_t sym, uint32_t modifiers)
{
    bool super_held = (modifiers & WLR_MODIFIER_LOGO) != 0;

    if (!super_held) {
        return false;
    }

    switch (sym) {
    case XKB_KEY_l:
    case XKB_KEY_L:
        /* Super+L: lock screen.
         * In Milestone 5 this calls PulseSessionManager.lock() over IPC.
         * For now we log the intent. */
        wlr_log(WLR_INFO, "Super+L: lock screen (Session Manager not yet wired)");
        return true;

    case XKB_KEY_q:
    case XKB_KEY_Q:
        /* Super+Q: quit compositor (temporary developer shortcut) */
        wlr_log(WLR_INFO, "Super+Q: terminating compositor");
        wl_display_terminate(server->wl_display);
        return true;

    default:
        return false;
    }
}

static void keyboard_handle_modifiers(struct wl_listener *listener, void *data)
{
    struct pulse_keyboard *keyboard =
        wl_container_of(listener, keyboard, modifiers);

    /* Forward modifier state to the focused client */
    wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr_keyboard);
    wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,
                                        &keyboard->wlr_keyboard->modifiers);
}

static void keyboard_handle_key(struct wl_listener *listener, void *data)
{
    struct pulse_keyboard *keyboard =
        wl_container_of(listener, keyboard, key);
    struct wlr_keyboard_key_event *event = data;

    /* Translate the raw evdev keycode to XKB keysym */
    uint32_t keycode = event->keycode + 8; /* evdev offset */
    const xkb_keysym_t *syms;
    int nsyms = xkb_state_key_get_syms(keyboard->wlr_keyboard->xkb_state,
                                         keycode, &syms);

    uint32_t modifiers =
        wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);

    bool handled = false;

    /* Only check compositor bindings on key-press, not key-release */
    if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        for (int i = 0; i < nsyms; i++) {
            if (handle_compositor_keybinding(keyboard->server, syms[i],
                                               modifiers)) {
                handled = true;
                break;
            }
        }
    }

    if (!handled) {
        /* Forward to the focused client */
        wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr_keyboard);
        wlr_seat_keyboard_notify_key(keyboard->server->seat,
                                      event->time_msec, event->keycode,
                                      event->state);
    }
}

static void keyboard_handle_destroy(struct wl_listener *listener, void *data)
{
    struct pulse_keyboard *keyboard =
        wl_container_of(listener, keyboard, destroy);

    wl_list_remove(&keyboard->modifiers.link);
    wl_list_remove(&keyboard->key.link);
    wl_list_remove(&keyboard->destroy.link);
    wl_list_remove(&keyboard->link);

    free(keyboard);
}

void input_new_keyboard(struct pulse_server *server,
                         struct wlr_input_device *device)
{
    struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(device);

    struct pulse_keyboard *keyboard = calloc(1, sizeof(*keyboard));
    if (!keyboard) {
        wlr_log(WLR_ERROR, "Out of memory allocating pulse_keyboard");
        return;
    }
    keyboard->server = server;
    keyboard->wlr_keyboard = wlr_keyboard;

    /* XKB keyboard layout — defaults to "us" with no special options.
     * Milestone 8 (Pulse Settings) will wire this to the keyboard settings
     * panel which reads from the Settings API. */
    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap *keymap = xkb_keymap_new_from_names(
        context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);

    wlr_keyboard_set_keymap(wlr_keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);

    /* Repeat rate: 25 keys/second, 600ms delay before repeat starts.
     * These are sensible desktop defaults. */
    wlr_keyboard_set_repeat_info(wlr_keyboard, 25, 600);

    keyboard->modifiers.notify = keyboard_handle_modifiers;
    wl_signal_add(&wlr_keyboard->events.modifiers, &keyboard->modifiers);

    keyboard->key.notify = keyboard_handle_key;
    wl_signal_add(&wlr_keyboard->events.key, &keyboard->key);

    keyboard->destroy.notify = keyboard_handle_destroy;
    wl_signal_add(&device->events.destroy, &keyboard->destroy);

    wlr_seat_set_keyboard(server->seat, wlr_keyboard);

    wl_list_insert(&server->keyboards, &keyboard->link);

    /* Update the seat capability flags to include keyboard */
    uint32_t caps = wlr_seat_get_capabilities(server->seat) |
                    WL_SEAT_CAPABILITY_KEYBOARD;
    wlr_seat_set_capabilities(server->seat, caps);

    wlr_log(WLR_DEBUG, "Keyboard attached: %s", device->name);
}

/* ---------------------------------------------------------------------------
 * Pointer
 * ------------------------------------------------------------------------- */

void input_new_pointer(struct pulse_server *server,
                        struct wlr_input_device *device)
{
    /* Attach the pointer to the cursor aggregator.
     * All attached pointers feed into the single wlr_cursor, which handles
     * pointer acceleration and multi-device coalescing. */
    wlr_cursor_attach_input_device(server->cursor, device);

    uint32_t caps = wlr_seat_get_capabilities(server->seat) |
                    WL_SEAT_CAPABILITY_POINTER;
    wlr_seat_set_capabilities(server->seat, caps);

    wlr_log(WLR_DEBUG, "Pointer attached: %s", device->name);
}

/* ---------------------------------------------------------------------------
 * Dispatch
 * ------------------------------------------------------------------------- */

void input_handle_new_input(struct wl_listener *listener, void *data)
{
    struct pulse_server *server =
        wl_container_of(listener, server, new_input);
    struct wlr_input_device *device = data;

    switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
        input_new_keyboard(server, device);
        break;
    case WLR_INPUT_DEVICE_POINTER:
        input_new_pointer(server, device);
        break;
    default:
        wlr_log(WLR_DEBUG, "Unhandled input device type: %d (will be ignored)",
                device->type);
        break;
    }
}
