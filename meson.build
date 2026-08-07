project(
  'pulselinux',
  'c',
  version: '0.1.0',
  license: 'MIT',
  meson_version: '>= 1.3.0',
  default_options: [
    'c_std=c17',
    'warning_level=2',
    'werror=true',
  ],
)

# ---------------------------------------------------------------------------
# Compiler flags
# ---------------------------------------------------------------------------
cc = meson.get_compiler('c')
libm = cc.find_library('m', required: true)

add_project_arguments(
  cc.get_supported_arguments([
    '-Wno-unused-parameter',   # wlroots callbacks often have unused params by design
    '-Wno-missing-field-initializers',
    '-D_POSIX_C_SOURCE=200809L',
    # wlroots is an unstable API: every wlroots header refuses to compile
    # unless WLR_USE_UNSTABLE is defined.
    '-DWLR_USE_UNSTABLE',
  ]),
  language: 'c',
)

# ---------------------------------------------------------------------------
# External dependencies — must all be present on the build machine.
# Version pins match Ubuntu 24.04 universe packages (see README.md).
# ---------------------------------------------------------------------------
wayland_server  = dependency('wayland-server',  version: '>= 1.22.0')
wayland_client  = dependency('wayland-client',  version: '>= 1.22.0')
wayland_protos  = dependency('wayland-protocols', version: '>= 1.32')
wlroots         = dependency('wlroots',          version: ['>= 0.17.0', '< 0.18.0'])
xkbcommon       = dependency('xkbcommon',        version: '>= 1.6.0')
libdrm          = dependency('libdrm',           version: '>= 2.4.120')
pixman          = dependency('pixman-1',         version: '>= 0.42.0')
libinput        = dependency('libinput',         version: '>= 1.25.0')
libudev         = dependency('libudev')
libseat         = dependency('libseat',          version: '>= 0.7.0')
cairo           = dependency('cairo',            version: '>= 1.18.0')

# ---------------------------------------------------------------------------
# Wayland protocol scanner — generates C stubs from XML protocol definitions
# ---------------------------------------------------------------------------
wayland_scanner_dep   = dependency('wayland-scanner', native: true)
wayland_scanner       = find_program(
  wayland_scanner_dep.get_variable('wayland_scanner'),
  native: true,
)
wayland_protos_dir    = wayland_protos.get_variable('pkgdatadir')

# ---------------------------------------------------------------------------
# Version metadata injected at compile time
# ---------------------------------------------------------------------------
conf = configuration_data()
conf.set_quoted('PULSE_VERSION',      meson.project_version())
conf.set_quoted('PULSE_VERSION_FULL', '@0@-dev'.format(meson.project_version()))
configure_file(
  output: 'pulse-version.h',
  configuration: conf,
)
version_inc = include_directories('.')

# ---------------------------------------------------------------------------
# Subdirectories
# ---------------------------------------------------------------------------
subdir('src/pulse-de')
subdir('src/pulse-panel')
# Future milestones add their subdir() calls here:
# subdir('src/pulse-runtime')
# subdir('src/pulse-api')
# subdir('src/pulse-apps/explorer')
# subdir('src/pulsepkg')

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
subdir('tests/unit')
