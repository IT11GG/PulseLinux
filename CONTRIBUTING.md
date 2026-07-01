# Contributing to PulseLinux

Thank you for your interest in contributing to PulseLinux. This document covers
the essential rules for keeping the project coherent as it grows.

---

## Ground rules

1. **The tree must compile at every commit.** A commit that breaks the build
   is reverted. No exceptions.
2. **No placeholders or stubs in committed code** — if a function cannot be
   implemented yet, it must not exist yet. Stub files are accepted only in
   test fixtures and must be clearly marked with `/* STUB */`.
3. **Respect the frozen specifications.** The design system, HIG, and platform
   architecture are frozen. Do not propose changes to them without a
   compelling implementation-discovered reason; "I prefer this approach" is
   not sufficient.
4. **One concern per commit.** Do not mix build-system changes with logic
   changes. Use the scope field in Conventional Commits.

---

## Commit format

We use [Conventional Commits](https://conventionalcommits.org) strictly.

```
<type>(<scope>): <short summary>

[optional body — explain WHY, not what]

[optional footer: BREAKING CHANGE, Closes #N]
```

**Types:** `feat` `fix` `build` `ci` `docs` `refactor` `test` `chore` `perf`

**Scopes:** `pulse-de` `compositor` `wm` `shell` `runtime` `ipc` `fs` `settings`
  `pkg` `theme` `sdk` `pulsepkg` `explorer` `settings-app` `store` `terminal`
  `build` `ci` `docs`

**Examples:**

```
feat(compositor): create wlroots output and renderer bootstrap

build(pulse-de): add Meson subdir for compositor with wlroots dependency

fix(wm): apply focus invariant — only one window focused at a time

test(compositor): add smoke test for output creation without display
```

---

## Pull request process

1. Fork → branch from `main` with a descriptive name (`feat/wm-snap-zones`,
   `fix/compositor-crash-on-vt-switch`).
2. Each PR should represent one vertical slice of work — a new feature or a
   specific bug fix, tested, building, and mergeable in isolation.
3. All CI checks must pass before review begins.
4. At least one maintainer review is required to merge.
5. Squash-merge into `main` — the squash commit message must follow the
   Conventional Commits format.

---

## Code style

**C (`src/pulse-de/`, `src/pulse-runtime/`, `src/pulse-api/`, `src/pulsepkg/`):**
- `clang-format` with the project `.clang-format` — run `scripts/format.sh` before committing.
- Functions and types are `snake_case`. Constants are `UPPER_SNAKE_CASE`.
- Every public API function has a doc comment (`/** ... */`) above its declaration.
- No global mutable state outside of explicitly documented singletons (the compositor struct, the IPC bus).
- `goto cleanup` is acceptable for C resource-cleanup paths; nested ternaries are not.

**Error handling:** Use `wlr_log(WLR_ERROR, ...)` for compositor-level errors.
Return `NULL` / `false` and log rather than `abort()` unless the situation is genuinely unrecoverable.

---

## Testing

- New compositor features require a corresponding unit test in `tests/unit/`.
- Integration tests that require a Wayland display run in the CI `wayland-headless` job.
- `ninja -C builddir test` must pass before a PR is opened.

---

## Design review

Any new UI surface, animation, or interaction **must** be checked against:
- Pulse HIG (`docs/design-system/pulse-hig.md`)
- Pulse DNA (`docs/design-system/pulse-dna.md`)
- Pulse Motion (`docs/design-system/01-pulse-motion.md`)

If implementation reveals a genuine conflict with a spec, open a `docs:` PR
to amend the spec first, with the implementation PR as a dependency.

---

## Getting help

Open a GitHub Discussion for questions. Use GitHub Issues only for confirmed bugs.
