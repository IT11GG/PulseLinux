# PulseLinux

[![Build Status](https://github.com/pulselinux/pulselinux/actions/workflows/build.yml/badge.svg)](https://github.com/pulselinux/pulselinux/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)
[![Conventional Commits](https://img.shields.io/badge/Conventional%20Commits-1.0.0-yellow.svg)](https://conventionalcommits.org)

PulseLinux is an open-source desktop operating system built from the ground up with its own Wayland compositor, desktop environment, package manager, and application ecosystem.

> The operating system that feels alive.

## Design Philosophy

PulseLinux is built around one idea: **the operating system feels alive, but it is never loud about it.** A calm, dark, OLED-first interface with a single identifiable motion language — the Pulse Beat — running through every surface. Full design specifications: [`docs/design-system/`](./docs/design-system/).

## Architecture

```
Kernel (Linux)
  └── System Services
        └── Pulse Runtime          (IPC, Service Manager)
              └── Pulse API        (FS, Settings, Package, Theme, Notifications)
                    └── PulseDE    (Wayland compositor + desktop shell)
                          └── Pulse Applications
```

See [`docs/platform/`](./docs/platform/) for the full platform architecture.

## Building

### Requirements

- Linux with a free VT or an existing Wayland session (for nested development)
- `gcc` ≥ 13 or `clang` ≥ 16
- `meson` ≥ 1.3 and `ninja` ≥ 1.11
- `libwlroots-dev` = 0.17.x (Ubuntu 24.04 universe)
- `libwayland-dev` ≥ 1.22, `wayland-protocols` ≥ 1.32
- `libxkbcommon-dev`, `libdrm-dev`, `libgbm-dev`
- `libinput-dev`, `libudev-dev`, `libpixman-1-dev`, `libseat-dev`

### Install dependencies (Ubuntu 24.04)

```bash
sudo apt-get install -y \
  meson ninja-build pkg-config gcc \
  libwlroots-dev libwayland-dev wayland-protocols \
  libxkbcommon-dev libdrm-dev libgbm-dev \
  libinput-dev libudev-dev libpixman-1-dev libseat-dev
```

### Build

```bash
meson setup builddir
ninja -C builddir
```

### Run

```bash
# Nested inside an existing Wayland session (development):
./builddir/src/pulse-de/pulse-de --backend=wayland

# From a VT (bare metal / VM):
./builddir/src/pulse-de/pulse-de
```

### Test

```bash
ninja -C builddir test
```

## Repository Structure

```
src/
  pulse-de/         Wayland compositor + PulseDE shell (C, wlroots 0.17)
  pulse-runtime/    IPC bus and base runtime (C)
  pulse-api/        Platform API subsystems (C)
  pulse-apps/       First-party applications (C + GTK4)
  pulse-sdk/        Application SDK headers
  pulsepkg/         Package manager (C)
docs/               Architecture, design system, platform specs
tests/              Unit and integration tests
scripts/hooks/      Git hook scripts
packaging/          deb / rpm / arch packaging
.github/workflows/  CI/CD pipelines
```

## Contributing

Read [CONTRIBUTING.md](./CONTRIBUTING.md) before opening a pull request.
We use [Conventional Commits](https://conventionalcommits.org).
The tree must remain buildable at every commit.

## Roadmap

[ROADMAP.md](./ROADMAP.md)

## License

MIT — see [LICENSE](./LICENSE).

## Security

[SECURITY.md](./SECURITY.md)
