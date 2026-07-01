# Pulse Icon System

**The icon language for PulseLinux — system glyphs, app icons, and file-type icons.**

---

## 1. Principle

Icons are drawn, not decorated. The icon system exists independently of the Beat — icons should read correctly as a static screenshot, in greyscale, at 16px. Motion (per Pulse Motion / Component Library) is something that happens *to* an icon in specific stateful contexts (charging, syncing); it is never baked into the icon's resting design. An icon that looks unfinished without animation is a design failure.

---

## 2. Geometry

- **Grid:** 24×24px base grid (matches most system icon conventions), with a 2px live-area inset — icon content stays within a 20×20px safe area.
- **Stroke weight:** 1.5px at 24px base size, scaling proportionally. Consistent stroke weight across the entire set is non-negotiable; mixed weights are the most common way an icon set looks unofficial.
- **Corner rounding:** 2px radius on all terminals and corners — a small, consistent rounding that echoes the UI's `--radius-control` language without becoming bubbly.
- **Style:** outlined (stroke-based), not filled, for system/UI glyphs (taskbar, settings, menus). Filled variants exist only for status/state glyphs that need to read at very small size (tray icons, notification dots) — see §4.

## 3. Construction rules

- Glyphs are built from a small constrained vocabulary of shapes: circles, straight lines, and the single 2px corner radius above. No bespoke bezier flourishes per icon — consistency across hundreds of icons (system glyphs, file types, app icons) depends on every icon being assembled from the same kit of parts, the way a typeface's letterforms share consistent terminals and curves.
- Two-tone is allowed only for app icons (§5), never for system/UI glyphs, which are always single-color (`currentColor`, inheriting `--pulse-text-primary` or `--pulse-text-secondary` depending on state) so they sit correctly on any surface.
- No gradients, no drop shadows, no glow baked into the icon artwork itself. Any glow an icon appears to have (e.g., a charging battery) is applied live by the component, per the Component Library — never authored into the SVG.

## 4. Status glyphs

A small filled-style sub-set used where outline icons would lose legibility at very small sizes: battery states, signal strength, sync status, notification dot, lock indicator. These use solid fills in `--pulse-text-primary`/`--pulse-text-secondary`/`--pulse-signal` only — never ember, never a separate status palette (e.g., generic green-for-good/red-for-bad). PulseLinux deliberately does not adopt the universal traffic-light color convention for status; clarity comes from iconography and label text, not from overloading red with "error" in addition to its brand role. (Exception: §6 destructive/error specifics below.)

## 5. App icons

- **Shape:** a fixed rounded-square "badge" container (consistent corner radius across every app icon — this container shape is as much a brand element as the Beat itself, the way macOS's squircle or Windows 11's badge shape is instantly recognizable independent of what's drawn inside it).
- **Container color:** each app gets one signature background color from a curated, OS-approved palette (warm neutrals, with `--pulse-signal` reserved exclusively for PulseLinux's own first-party "core" apps — Settings, Store, Explorer, Terminal — so a glance at the dock tells you which icons are system-level versus third-party at a glance).
- **Glyph:** single-color or two-tone mark, centered, built from the same construction rules as system glyphs (§3), scaled up.
- Third-party/Store apps may use their own brand colors for the container, but must keep the same badge shape — this is what makes Pulse Store screenshots still look cohesive even when full of different developers' apps.

## 6. Error and destructive iconography

Error and destructive states use `--pulse-ember` for the icon itself (not Signal — Signal stays reserved for the "alive" brand role, not for alarm), with the single Response beat defined in Component Library §2/§7 providing emphasis on arrival, never a sustained pulse. An error icon that pulses forever is treated as a bug, per Pulse Motion §4's ban on panic-cue speedups — errors get one calm beat, then sit still so they can be read.

## 7. File-type icons

Flat, rounded-rectangle "document" base shape shared by every file type, distinguished by a small corner-badge glyph (per-format) and a one-letter or short-mark identifier where useful (matching the construction rules in §3). No skewed 3D page-curl effects, no photographic thumbsnails baked into the icon chrome — actual file previews (e.g., image thumbnails in Pulse Explorer) render inside this same consistent frame rather than replacing it, so a folder full of mixed file types still reads as one coherent grid.

## 8. What this system explicitly avoids

- Photorealistic or skeuomorphic icon detail (shadows, bevels, gloss) — contradicts the flat, high-contrast, minimal language set by the theme spec.
- A "filled" icon style as the default for system glyphs — outlined keeps the UI feeling light despite the pure-black canvas; an all-filled icon set on OLED black gets visually heavy fast.
- Per-icon creative license on stroke weight, corner radius, or color outside this document — any contributor-submitted icon that breaks §2–3's construction rules should be revised before merge, the same way an off-brand font weight would be.
