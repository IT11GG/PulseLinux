# tests/unit/meson.build
#
# Unit tests that do NOT require a Wayland display or a running compositor.
# These are the tests that run in CI on every push.
#
# Tests that need a display (full compositor startup) live in
# tests/integration/ and run in the CI headless-Wayland job.

# ---------------------------------------------------------------------------
# test_version: sanity-checks that the version string is non-empty and
# matches what Meson reported. Trivial, but proves the test runner is wired.
# ---------------------------------------------------------------------------
test_version_src = files('test_version.c')

test_version = executable(
  'test_version',
  test_version_src,
  include_directories: version_inc,
  install: false,
)

test('version string is set', test_version)

# ---------------------------------------------------------------------------
# test_decoration_colors: verifies frozen design-token colour constants
# match the hex values in the theme spec. No Wayland/wlroots dependency.
# ---------------------------------------------------------------------------
test_decoration_colors = executable(
  'test_decoration_colors',
  files('test_decoration_colors.c'),
  dependencies: [libm],
  install: false,
)

test('decoration colours match theme spec', test_decoration_colors)

# ---------------------------------------------------------------------------
# test_grab: unit tests for the interactive grab state machine.
# Tests move, resize, min-size clamping, double-grab guard, idempotent end.
# No Wayland/wlroots dependency — pure C logic test.
# ---------------------------------------------------------------------------
test_grab = executable(
  'test_grab',
  files('test_grab.c'),
  install: false,
)

test('grab state machine', test_grab)

# ---------------------------------------------------------------------------
# test_focus_stacking: Milestone 4 — is_mapped tracking, focus transfer
# skipping unmapped windows, Z-order invariant, cursor enter dedup.
# No Wayland/wlroots dependency.
# ---------------------------------------------------------------------------
test_focus_stacking = executable(
  'test_focus_stacking',
  files('test_focus_stacking.c'),
  install: false,
)

test('focus and stacking state machine', test_focus_stacking)

# ---------------------------------------------------------------------------
# test_clock: Milestone 5 — clock string formatting, zero-padding,
# buffer bounds, design token cross-checks. No Wayland/Cairo dependency.
# ---------------------------------------------------------------------------
test_clock = executable(
  'test_clock',
  files('test_clock.c'),
  install: false,
)

test('clock string formatting', test_clock)
