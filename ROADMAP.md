# PulseLinux Roadmap

This roadmap tracks high-level milestones in order of implementation.
Milestones are closed when the corresponding code lands in `main` in a
buildable, testable state. Each milestone maps to one or more git commits.

---

## Milestone 0 — Repository foundation ⬛ IN PROGRESS

- [x] Git repository with governance files
- [x] Meson build system skeleton
- [x] GitHub Actions CI/CD
- [x] Pre-commit hooks and clang-format
- [x] Design system and platform specs imported to `docs/`

---

## Milestone 1 — PulseDE Core: minimal compositor ⬜ PENDING

Goal: `pulse-de` binary that opens a Wayland session, clears the screen to
`--pulse-void` (#050505), and exits cleanly on Ctrl+C.

- [ ] wlroots compositor init (backend, renderer, allocator)
- [ ] Wayland display socket creation
- [ ] Output configuration and frame rendering
- [ ] Basic event loop
- [ ] Clean shutdown sequence

---

## Milestone 2 — PulseDE: desktop background and cursor ⬜ PENDING

- [ ] Solid colour background matching `--pulse-void`
- [ ] Custom Pulse cursor (SVG → compiled XCursor)
- [ ] Input handling (keyboard + pointer via libinput)
- [ ] XKB keyboard configuration

---

## Milestone 3 — PulseDE: window management foundation ⬜ PENDING

- [ ] XDG shell implementation (xdg_surface, xdg_toplevel)
- [ ] Window focus model (focus-follows-pointer for now)
- [ ] Single focused-window Ambient Beat hairline (compositor-side)
- [ ] Keyboard focus and seat management

---

## Milestone 4 — PulseDE: Pulse Shell ⬜ PENDING

- [ ] Layer shell protocol (wlr-layer-shell-unstable-v1)
- [ ] Taskbar surface (bottom, floating glass strip)
- [ ] Start button + pinned app icons
- [ ] System clock (tray right)

---

## Milestone 5 — Pulse Runtime ⬜ PENDING

- [ ] IPC bus (Unix domain socket, message framing)
- [ ] Service Manager with dependency resolution
- [ ] Session Manager state machine

---

## Milestone 6 — Pulse API subsystems ⬜ PENDING

- [ ] Filesystem API service
- [ ] Settings API service (schema-first)
- [ ] Theme Engine service (token resolution, Reduce Motion)
- [ ] Notifications service

---

## Milestone 7 — Pulse Explorer ⬜ PENDING

- [ ] GTK4 file manager built against Pulse SDK
- [ ] Sidebar + content navigation
- [ ] File grid with Pulse icon system

---

## Milestone 8 — Pulse Settings ⬜ PENDING

- [ ] GTK4 settings application built against Pulse SDK
- [ ] Network, Display, Accessibility panels
- [ ] Live Reduce Motion / Reduce Transparency toggle

---

## Future (not yet scheduled)

- PulsePkg package manager
- Pulse Store
- Pulse Terminal
- Pulse AI assistant
- Gaming mode
- HDR and OLED tuning
- Multi-monitor improvements
- Wayland XWayland support
- Pulse recovery mode
- Secure Boot integration
