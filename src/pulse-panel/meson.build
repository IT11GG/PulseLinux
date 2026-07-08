# src/pulse-panel/meson.build
# Pulse Panel — the first visible Pulse Shell component.
# A standalone Wayland client that uses wlr-layer-shell to anchor a panel
# at the bottom of every output. The compositor (pulse-de) must be running
# and advertising layer-shell before this client can connect.

# ---------------------------------------------------------------------------
# Protocol generation (client side — different from the server-side headers
# generated in pulse-de/meson.build)
# ---------------------------------------------------------------------------
xdg_shell_client_xml = wayland_protos_dir / 'stable/xdg-shell/xdg-shell.xml'

xdg_shell_client_h = custom_target(
  'panel-xdg-shell-client-header',
  output: 'xdg-shell-client-protocol.h',
  input:  xdg_shell_client_xml,
  command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
)

xdg_shell_client_c = custom_target(
  'panel-xdg-shell-client-code',
  output: 'xdg-shell-protocol.c',
  input:  xdg_shell_client_xml,
  command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
)

layer_shell_xml = wlroots.get_variable('pkgdatadir') / 'protocols/wlr-layer-shell-unstable-v1.xml'

layer_shell_client_h = custom_target(
  'panel-layer-shell-client-header',
  output: 'wlr-layer-shell-unstable-v1-client-protocol.h',
  input:  layer_shell_xml,
  command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
)

layer_shell_client_c = custom_target(
  'panel-layer-shell-client-code',
  output: 'wlr-layer-shell-unstable-v1-protocol.c',
  input:  layer_shell_xml,
  command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
cairo_dep  = dependency('cairo',   version: '>= 1.18.0')
pixman_dep = dependency('pixman-1', version: '>= 0.42.0')

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
panel_sources = files(
  'main.c',
  'panel.c',
  'clock.c',
)

panel_generated = [
  xdg_shell_client_h,
  xdg_shell_client_c,
  layer_shell_client_h,
  layer_shell_client_c,
]

panel_inc = include_directories(
  '.',
  meson.current_build_dir(),
)

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
executable(
  'pulse-panel',
  sources: [panel_sources, panel_generated],
  include_directories: [panel_inc, version_inc],
  dependencies: [
    wayland_client,
    cairo_dep,
    pixman_dep,
  ],
  install: true,
  install_dir: get_option('bindir'),
)
