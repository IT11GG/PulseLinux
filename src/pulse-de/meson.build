# src/pulse-de/meson.build
# PulseDE Wayland compositor — Milestone 1: minimal compositor skeleton

# ---------------------------------------------------------------------------
# Wayland protocol generation
# wayland-scanner generates the C glue code from the upstream XML protocol
# definitions. These generated files are compiled into the compositor.
# ---------------------------------------------------------------------------
xdg_shell_xml = wayland_protos_dir / 'stable/xdg-shell/xdg-shell.xml'

xdg_shell_server_h = custom_target(
  'xdg-shell-server-header',
  output: 'xdg-shell-protocol.h',
  input:  xdg_shell_xml,
  command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
)

xdg_shell_c = custom_target(
  'xdg-shell-source',
  output: 'xdg-shell-protocol.c',
  input:  xdg_shell_xml,
  command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
)

# Layer-shell protocol (external; wlroots ships a copy in its own protocols dir)
# wlr-layer-shell is vendored into protocols/ because wlroots does not
# expose a stable pkg-config variable pointing to its protocol directory.
layer_shell_xml = meson.project_source_root() / 'protocols' / 'wlr-layer-shell-unstable-v1.xml'

layer_shell_server_h = custom_target(
  'layer-shell-server-header',
  output: 'wlr-layer-shell-unstable-v1-protocol.h',
  input:  layer_shell_xml,
  command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
)

layer_shell_c = custom_target(
  'layer-shell-source',
  output: 'wlr-layer-shell-unstable-v1-protocol.c',
  input:  layer_shell_xml,
  command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
)

# xdg-decoration protocol (server-side decorations — SSD)
# This protocol lives in wayland-protocols, not wlroots' own protocols dir.
xdg_decoration_xml = wayland_protos_dir / 'unstable/xdg-decoration/xdg-decoration-unstable-v1.xml'

xdg_decoration_server_h = custom_target(
  'xdg-decoration-server-header',
  output: 'xdg-decoration-unstable-v1-protocol.h',
  input:  xdg_decoration_xml,
  command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
)

xdg_decoration_c = custom_target(
  'xdg-decoration-source',
  output: 'xdg-decoration-unstable-v1-protocol.c',
  input:  xdg_decoration_xml,
  command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
)

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
pulse_de_sources = files(
  'src/main.c',
  'src/server.c',
  'src/output.c',
  'src/input.c',
  'src/cursor.c',
  'src/xdg-shell.c',
  'src/decoration.c',
  'src/grab.c',
  'src/layer.c',
)

pulse_de_generated = [
  xdg_shell_server_h,
  xdg_shell_c,
  layer_shell_server_h,
  layer_shell_c,
  xdg_decoration_server_h,
  xdg_decoration_c,
]

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
pulse_de_inc = include_directories(
  'include',
)
# Generated protocol headers (.h files from custom_target) are automatically
# accessible because Meson adds the target's build directory to the include
# path when a generated file appears in the sources list. Passing
# meson.current_build_dir() explicitly is both unnecessary and rejected by
# Meson >= 1.3 as an absolute path inside the source tree.

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
executable(
  'pulse-de',
  sources: [pulse_de_sources, pulse_de_generated],
  include_directories: [pulse_de_inc, version_inc],
  dependencies: [
    wayland_server,
    wlroots,
    xkbcommon,
    libdrm,
    pixman,
    libinput,
    libudev,
    libseat,
  ],
  install: true,
  install_dir: get_option('bindir'),
)
