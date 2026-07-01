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
