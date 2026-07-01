# Pulse Accessibility Rules

**Accessibility constraints that apply across every other Pulse spec. Where this document and any other Pulse spec conflict, this document wins.**

---

## 1. Principle

"Alive" cannot come at the cost of usable. A motion and color identity built around a dark, high-contrast, red-accented, gently-animated interface carries real, well-documented risks — motion sensitivity, photosensitivity, color-vision deficiency, low-vision contrast needs — and PulseLinux treats designing around those risks as core to the brief, not as a bolt-on compliance pass at the end. An OS that's beautiful for most people and actively unusable or uncomfortable for others is not "the easiest Linux OS ever created."

---

## 2. Motion

- **System-wide "Reduce Motion" setting, on by default for screen-reader users and respected everywhere.** When enabled: every Ambient Beat (Pulse Motion §2.2) is replaced by a single static indicator of the same state — e.g., a charging battery shows a static bolt glyph in `--pulse-signal` rather than a breathing glow; an indeterminate loading bar shows a static partially-filled bar with a small textual "Loading…" label rather than a traveling band. Response beats are reduced to a simple, instant state change (no scale/glow animation at all) rather than removed silently — the user should still get *a* signal that their action registered, just not an animated one.
- PulseLinux also respects the standard OS-level "prefers-reduced-motion" signal from the underlying platform/session for any app or web content rendered inside it, independent of the Pulse-specific toggle.
- No Pulse animation, even at full motion settings, may exceed 3 flashes per second in any region of the screen, and the Beat's actual tempo (~0.4Hz Ambient, single-shot Response) sits far below any seizure-risk threshold by design — this is a hard ceiling that overrides any future request to "make it pulse faster" for emphasis (see Pulse Motion §4, which already disallows this for design reasons; this section disallows it for safety reasons, independently).
- Reduce Motion does not mean "remove the OS's identity for some users." Static-but-correct equivalents are specified above precisely so the *information* the Beat conveys (something is charging, something is loading, this window is focused) is never lost — only the animation is.

---

## 3. Color and contrast

- Signal red (`--pulse-signal`) against `--pulse-void`/`--pulse-surface-1` is verified to meet WCAG AA contrast for text and meaningful UI elements at the sizes specified in the theme spec; any future token change to either color must be re-verified, not assumed safe because the original pairing was.
- PulseLinux's status iconography (Icon System §4) deliberately does not rely on a red/green color pairing to convey good/bad state, both for brand-coherence reasons stated there and because red/green is the single most common confusion pair in color-vision deficiency. Every status indicator pairs color with a distinct shape or icon so state is never conveyed by hue alone.
- A "High Contrast" mode increases all surface-to-surface contrast steps (Void/Surface-1/2/3) beyond the default subtle elevation system, and thickens default 1px hairlines to 2px — available as a single Settings toggle, not a separate theme to maintain.
- Color is never the only signal for a focused vs. unfocused window (Component Library §6) — focused windows also get the brighter static hairline at minimum, even under Reduce Motion, so focus state remains legible without relying on the animated cue.

---

## 4. Text and readability

- Body text never renders below 13px effective size anywhere in first-party Pulse apps; a system-wide text scaling setting (100–200%) is supported and every layout in the Component Library must reflow rather than truncate or overlap at 150%+ scale.
- `--pulse-text-secondary` and `--pulse-text-tertiary` are for genuinely secondary information only — no critical action or label is ever set exclusively in the lowest-contrast tertiary tone.

---

## 5. Sound

- Every sound in Pulse Sound Design has a corresponding visual equivalent (a Component Library notification, banner, or state change) — sound is additive, never the sole channel for any piece of information, for users who are deaf, hard of hearing, or simply have sound muted (the system's normal default for many routine actions, per Sound Design §3).
- Screen-reader output is a separate channel from Pulse Sound's tones and is never suppressed by Do Not Disturb/Focus mode in the way decorative tones are (Sound Design §6) — DND silences *notification* sound, never assistive narration the user has actively requested.

---

## 6. Input and interaction

- Every interactive element from the Component Library (buttons, toggles, inputs, window controls) is fully operable by keyboard alone, with a visible focus indicator that meets the same contrast requirements as §3 — the focus ring is not allowed to rely on the Signal glow alone if Reduce Motion or Reduce Transparency is active.
- Hit targets for all touch-relevant controls (tablet mode, touch displays) meet a 44×44px minimum regardless of how small the visual control looks at the theme's default density.
- A "Reduce Transparency" setting flattens all glass/blur surfaces (taskbar, overlays, notification center) to solid `--pulse-surface-2`, for users sensitive to blurred/translucent motion or who need maximum text legibility against busy wallpapers.

---

## 7. Process

Every new component proposed against the Component Library must specify its Reduce Motion fallback and its keyboard interaction model in the same document/PR that proposes its default behavior — these are not follow-up tickets. A component without both is incomplete, the same way a component without an agreed color token would be.
