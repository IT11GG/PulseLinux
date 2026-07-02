# Building PulseDE

**Current milestone: Milestone 2 — minimal compositor with window decorations**

At this milestone, building PulseDE produces a Wayland compositor that:
- Clears the screen to `#050505` (--pulse-void)
- Accepts XDG shell clients (Wayland-native apps like `foot`, `weston-terminal`)
- Shows a visible 2px Signal-red border (`#FF2438`) on the focused window
- Shows a dim border on unfocused windows
- Moves the hardware cursor
- Responds to click-to-focus between windows
- Exits cleanly on Super+Q

---

## Dependencies

Ubuntu 24.04 (Noble Numbat):

```bash
sudo apt-get update
sudo apt-get install -y \
    meson ninja-build pkg-config gcc \
    libwlroots-dev \
    libwayland-dev wayland-protocols \
    libxkbcommon-dev \
    libdrm-dev libgbm-dev \
    libinput-dev libudev-dev \
    libpixman-1-dev \
    libseat-dev
```

> **Note:** `libwlroots-dev` is in Ubuntu 24.04's **universe** repository.
> If it's not found, enable universe first:
> `sudo add-apt-repository universe && sudo apt-get update`

---

## Build

```bash
git clone https://github.com/pulselinux/pulselinux
cd pulselinux
meson setup builddir --buildtype=debug
ninja -C builddir
```

Build output: `builddir/src/pulse-de/pulse-de`

---

## Run

### Nested (inside an existing Wayland session — recommended for development)

This is the safest way to develop: PulseDE opens as a window inside your
current GNOME/KDE/Sway session.

```bash
./builddir/src/pulse-de/pulse-de --backend=wayland
```

The compositor will announce itself on a new Wayland socket (e.g.
`wayland-1`). Open a second terminal and launch a client into it:

```bash
WAYLAND_DISPLAY=wayland-1 foot
# or
WAYLAND_DISPLAY=wayland-1 weston-terminal
```

You should see a terminal window appear inside PulseDE with a red top border.
Click between windows to see focus change (border colour shifts).

### From a VT (bare metal or VM — production-like)

Switch to a free virtual terminal (Ctrl+Alt+F3), log in, then:

```bash
cd /path/to/pulselinux
./builddir/src/pulse-de/pulse-de
```

The compositor takes over the display. Open another VT session or use
`seatd` / `logind` for privilege-free DRM access.

### Keyboard shortcuts (Milestone 2)

| Key | Action |
|-----|--------|
| Super+Q | Quit compositor |
| Super+L | Lock (logs intent; Session Manager not yet wired) |

---

## Run unit tests

```bash
ninja -C builddir test
```

Expected output:
```
1/2 version string is set              OK   0.01s
2/2 decoration colours match theme spec OK   0.01s

Ok:         2
Expected Fail:  0
Fail:       0
```

---

## Verify the Milestone 2 runnable state

The milestone is verified when all of the following are true on a real machine:

1. `meson setup builddir && ninja -C builddir` exits with code 0.
2. `ninja -C builddir test` shows 0 failures.
3. `./builddir/src/pulse-de/pulse-de --backend=wayland` opens a compositor
   window filled with `#050505`.
4. `WAYLAND_DISPLAY=wayland-1 foot` (or `weston-terminal`) launches a client
   window with a visible red top border.
5. Clicking a second client window shifts the red border to it.
6. Super+Q exits cleanly with log message "PulseDE exited cleanly".

---

## Troubleshooting

**`wlr_backend_autocreate: no backend available`**
→ Ensure `WAYLAND_DISPLAY` or `DISPLAY` is set for nested mode, or run from
a VT with a seat (logind/seatd).

**`Failed to create Wayland socket`**
→ Another compositor is using `wayland-0`. The socket auto-increments —
check the log for which socket was chosen (`WAYLAND_DISPLAY=wayland-N`).

**Black window, no client content visible**
→ The background rect is rendering but no client has connected. Launch a
client in a second terminal pointing at the correct `WAYLAND_DISPLAY`.

**Client appears without borders**
→ The client may not support xdg-decoration. Check logs for
`xdg-decoration: client requested client-side decorations`. This is known for
some GTK3 apps — GTK4 and most terminal emulators use SSD correctly.
