# Pulse Human Interface Guidelines (Pulse HIG)

**Version 1.0 — the platform rulebook for PulseLinux.**

This document, alongside Pulse DNA, the Pulse design tokens, Pulse Motion, Pulse Animation Guidelines, the Pulse Component Library, the Pulse Icon System, Pulse Sound Design, and Pulse Accessibility Rules, forms the complete Pulse design foundation. Nothing in PulseDE, any first-party app, or any Pulse Store submission should contradict it without the contradiction being resolved here first.

---


**The platform rulebook for PulseLinux. Every first-party app, every PulseDE surface, and every third-party app submitted to Pulse Store should be checkable against this document. Where this document is silent, fall back to the Pulse Component Library, Pulse Motion, and Pulse Accessibility Rules — never invent a pattern this document hasn't sanctioned without first amending it here.**

This is a living platform document, not a style guide for one release. Treat additions to it the way you'd treat additions to a programming language's standard library: rare, deliberate, and only after the alternative (working around a gap with a one-off pattern) has been tried and found wanting.

---

## 0. UX principles

Five principles govern every decision in this document. When a new situation isn't explicitly covered, resolve it against these, in order:

1. **A lifelong Windows user should never be lost.** Every core interaction pattern (right-click for context, a Start-like launcher, a taskbar, a notification area, drag-to-snap) must have an instantly familiar entry point, even though its visual execution is entirely PulseLinux's own. Familiarity of *structure*, originality of *execution* — never the reverse.
2. **No interaction requires the terminal.** If a first-party app or system flow's only path to completing a task is "open Pulse Terminal," that's a product gap, not an acceptable power-user shortcut. Power users keep the terminal as an option; beginners must never need it.
3. **One obvious way to do a common thing.** PulseLinux does not ship three different ways to close a window or open settings "for flexibility." Optionality is for power users via shortcuts and Pulse AI, layered on top of one clear default path, not a fork in the primary path itself.
4. **The OS is calm until it has something to say.** Idle PulseLinux is quiet — visually and audibly — per Pulse Motion and Pulse Sound Design. Nothing in this HIG should reintroduce decorative noise (badges, banners, animated chrome) that those documents deliberately ruled out.
5. **Every screen should still make sense with the Beat turned off.** Reduce Motion is not a degraded experience; see Pulse Accessibility Rules. If a flow only makes sense because of an animation, the flow is underspecified, not the animation under-built.

---

## 1. Window behavior

### 1.1 Window anatomy
Every window uses the structure defined in Pulse Component Library §6: 12px corner radius, 36–38px titlebar, focused-only Ambient hairline. Title text is left-aligned (not centered) following the "familiar to Windows users" principle — centered titles are a macOS convention that reads as foreign to the primary migration audience this OS targets.

### 1.2 Window controls
Minimize / Maximize / Close render as the three `tc-dot` controls at the **right** edge of the titlebar, in that left-to-right order (minimize, maximize, close) — matching Windows convention, deliberately not macOS's left-aligned traffic lights, again per Principle 1. Close is always the rightmost, always the only one that tints toward `--pulse-signal` on hover, so "the dangerous one" is spatially and chromatically consistent across the entire OS.

### 1.3 Resize and snap
- Dragging a window to a screen edge previews a snap zone (a translucent `--pulse-surface-2` overlay with a `--radius-window` outline) before release — the preview itself uses ordinary motion (Animation Guidelines §5), never the Beat.
- Snap layout options on hover/hold of the maximize control: half (left/right), quarter (four corners), and thirds — exposed as a small grid picker, matching the now-familiar Windows 11 snap-layout pattern, restyled in Pulse surfaces.
- Snapped windows remember their zone; replacing one snapped window with another from the same zone re-snaps automatically without the user re-dragging.

### 1.4 Window states
Standard: normal, maximized, minimized, snapped, fullscreen (chrome fully hidden, recoverable via a top-edge hover reveal using ordinary motion), and tiled (3+ windows in a saved layout, see §1.5).

### 1.5 Multitasking
- **Workspace Overview** (triggered by a dedicated key, trackpad gesture, or Start Menu) shows all open windows as live thumbnails over a dimmed desktop, grouped by virtual desktop. Thumbnails use ordinary scale/fade motion; only the currently-focused thumbnail (if visible) carries the Ambient hairline, consistent with §1.2 of the Component Library.
- **Virtual desktops** are horizontally arranged, named (not just numbered, though numbers are shown small), and switchable via gesture, keyboard shortcut, or the Overview strip. Each desktop can carry an independent wallpaper variant (same wallpaper family, not unrelated images, to avoid visual incoherence when switching).
- **Tiled layouts**: a saved arrangement of 2–4 windows users can name and recall (e.g., "Writing," "Dev") — addressing a real beginner pain point (manually re-arranging the same 3 windows every day) without requiring any tiling-window-manager knowledge.

---

## 2. Navigation patterns

### 2.1 Within first-party apps
A consistent **sidebar + content** pattern is the default for any app with more than one section (Settings, Explorer, Store, Photos) — sidebar items use the Icon System's outlined glyphs plus label, with the active item indicated by a filled `--pulse-surface-2` pill background, never by a colored left-border tab (too web-app-circa-2015). Apps with a single flat list of content (Calculator, simple utilities) skip the sidebar entirely rather than forcing the pattern.

### 2.2 Back/forward
Apps with hierarchical navigation (Explorer, Settings, Store) provide a consistent back control in the same top-left position across all apps — never relying solely on a swipe gesture or browser-style alt+arrow that a Windows-background user wouldn't expect.

### 2.3 Search as a first-class navigation method
Every major app (Settings, Explorer, Store) surfaces a search field in the same top position, and **system-wide search** (via Start Menu, see §X in the future Start Menu prototype) can deep-link directly into a specific Settings panel or app section, not just launch the app at its default screen. This is a deliberate, opinionated stance: search should resolve intent, not just locate an app icon.

### 2.4 Breadcrumbs vs. back-stack
File-system-like depth (Explorer) uses a breadcrumb trail in the header, individually clickable, with a dropdown chevron on truncation for deep paths. Linear task flows (a Settings sub-panel, a wizard) use the back control instead — breadcrumbs are for *places*, back is for *steps*; conflating the two patterns is a common HIG violation to watch for in review.

---

## 3. Dialogs

### 3.1 Types
- **Alert** — single message, 1–2 actions, no further nesting allowed. Used for confirmations and simple errors.
- **Modal panel** — larger, can contain a small form (e.g., "Rename," "New folder," "Add network manually"). Always dismissable via Escape and an explicit close control, never trap-only.
- **Sheet** — slides up from the bottom third of the parent window for contextual, app-scoped tasks (e.g., "Share," export options) — anchored to the window that spawned it, not centered on the full display, so its origin is spatially obvious.

### 3.2 Rules
- A dialog never spawns a second modal dialog on top of itself. If a deeper decision is required, the first dialog's content changes in place (a "step 2" state) rather than stacking — stacked modals are explicitly disallowed anywhere in first-party PulseLinux software.
- Destructive actions in any dialog use the Destructive button variant (Component Library §2) and require the action's specific consequence to be named in the body text ("Delete 14 files permanently" — not generic "Are you sure?").
- Dialogs use ordinary entrance motion (fade + slight scale-up from 96%, ~160ms) — never the Beat (Animation Guidelines §5); a dialog's job is to focus attention through stillness, not draw it through motion.
- Default focus lands on the non-destructive action for any dialog offering a destructive choice, so a reflexive Enter-key press never executes the dangerous path.

---

## 4. Menus

### 4.1 Menu bar vs. in-window menu
PulseLinux apps do not use a persistent top-of-screen global menu bar (a macOS convention unfamiliar to the target migration audience per Principle 1). Each app window carries its own menu access — either a compact "hamburger"-equivalent (the Pulse "more" glyph, three horizontal lines per Icon System construction rules) in the titlebar, or direct always-visible controls for apps simple enough not to need a menu at all.

### 4.2 Dropdown menus
Surface: `--pulse-surface-2`, `--radius-control` corners (not the larger window radius — menus are a smaller-scale surface), 1px `--pulse-line` border, glass blur consistent with the taskbar/overlay treatment defined in the theme spec. Items: 13px Inter 500, 8px vertical padding, hover state is a flat `--pulse-surface-3` highlight (no Beat — see Animation Guidelines §5, this is ordinary 100ms fade). Destructive items (e.g., "Delete") render in `--pulse-ember`, separated from neutral items by a hairline divider, never just by color alone (Accessibility Rules §3).

### 4.3 Context menus (right-click)
Identical visual treatment to dropdown menus (§4.2) — there is exactly one menu surface style in PulseLinux, reused everywhere, which is itself part of what makes the platform feel coherent. Context menus open anchored to the cursor position, flipping orientation automatically to stay on-screen, and always include the most-relevant action as the first item (not alphabetical — task-relevance ordering, matching the "search resolves intent" philosophy in §2.3).

---


## 5. Settings layout

### 5.1 Structure
Pulse Settings follows a fixed two-level structure: a left sidebar of top-level categories (System, Network, Personalization, Apps, Accounts, Privacy & Security, Updates, Accessibility, About), each opening a list of rows in the content area, following the sidebar + content pattern from §2.1. No third level beyond a sub-panel — if a setting needs more depth than category → panel → sub-panel, it's a sign the panel needs splitting, not nesting deeper.

### 5.2 Row anatomy
Every settings row matches Component Library §13's card containment logic: label + optional description on the left, control (toggle, dropdown, button) on the right, consistent 14px/18px vertical/horizontal padding, hairline divider between rows, never between a row's label and its own control. Rows that lead to a sub-panel show a chevron, not a toggle, so destination-rows and state-rows are never visually ambiguous.

### 5.3 Search
A persistent search field at the top of the Settings sidebar filters both category names and individual setting labels in real time, and selecting a result deep-links directly into that row with a brief highlight (ordinary motion, a single 600ms fade-pulse on the row background using `--pulse-surface-3`, not the Beat — Settings rows are static surfaces per Component Library §13).

### 5.4 Defaults and reset
Every settings panel that can be meaningfully misconfigured includes a scoped "Reset this section to default" action at the bottom — never a single global "factory reset" buried as the only undo path, which is a common source of beginner anxiety ("what if I break something") this HIG explicitly designs against.

---

## 6. Notifications

### 6.1 Notification Center
A persistent log of all notifications, opened via a taskbar tray icon or edge-swipe, living in a right-anchored panel using the same glass/blur treatment as the taskbar. Notifications group by app (collapsed by default beyond 3 from the same source), newest at top, with a per-app and global "Clear all" — never auto-deleted silently, since silently vanishing notifications undermines trust in the log.

### 6.2 Toasts (in-the-moment notifications)
Match Component Library §7 exactly: ordinary slide-in motion, one Response beat on the leading edge, then fully static, auto-dismiss after 6 seconds unless it contains an action (a "Reply" or "Undo" affordance), in which case it persists until manually dismissed or actioned.

### 6.3 Priority and grouping
Three levels only — **Passive** (logged silently to Notification Center, no toast, no sound: background app updates, minor sync events), **Standard** (toast + Notification Center entry, no sound unless explicitly enabled per-app), **Important** (toast + Notification Center + Pulse Low tone per Sound Design §2 — reserved for security, account, and explicit user-requested alerts like timers). Apps declare a notification's priority; users can always re-categorize per-app in Settings, but the three-tier ceiling is fixed platform behavior, not something an app can invent a fourth tier around.

### 6.4 Do Not Disturb / Focus
Suppresses Standard and Important toasts and all sound (Sound Design §6) while still logging everything to Notification Center, with a visible taskbar indicator showing DND is active — DND must never be a silent, undiscoverable state the user forgets they left on.

---

## 7. Touch interactions

### 7.1 Principle
PulseLinux is desktop-first, touch-aware — not touch-primary. Every control sized for mouse use must still meet Accessibility Rules §6's 44×44px minimum hit target when a touch-capable display is detected, achieved through generous invisible padding around visually compact controls rather than visually enlarging the whole UI.

### 7.2 Gestures
A small, consistent gesture vocabulary, never overloaded with app-specific meanings: single tap (activate), long-press (context menu, equivalent to right-click), two-finger pan (scroll), pinch (zoom, only where zoom is meaningful), three-finger swipe up (Workspace Overview), three-finger swipe left/right (switch virtual desktop), edge-swipe from right (Notification Center), edge-swipe from left (Quick Settings is reachable here too, mirrored — see §X Quick Settings in future prototypes). No 4+ finger gestures in first-party use — they're hard to discover and easy to trigger accidentally on smaller touch devices.

### 7.3 Touch feedback
Tap feedback uses the same Response beat as a mouse click on the same control (Component Library §2) — touch does not get a separate animation language; one input modality's feedback is just triggered by a different event source.

---

## 8. Tablet mode

### 8.1 Trigger
Automatic on 2-in-1 hardware fold/detach detection, or manually toggled from Quick Settings. Transition between modes uses a single ordinary 200ms layout crossfade — never the Beat, since this is a structural mode change, not a "something is alive" moment.

### 8.2 What changes
- Taskbar grows slightly taller (52px → 64px) and pinned icons gain more inter-icon spacing, both purely for touch target comfort (§7.1) — no new visual style is introduced.
- Start Menu and Notification Center open full-screen instead of as floating panels, since floating glass panels lose legibility at typical tablet hold-distance and one-handed reachability.
- Window snapping becomes the primary multitasking method (true freeform dragging is suppressed in favor of an explicit split-view gesture) since freeform window dragging is a poor touch interaction.
- On-screen keyboard follows the same surface/radius/type tokens as the rest of the system — it must never look like a bolted-on accessibility feature; it's core to tablet mode's identity.

---

## 9. Accessibility (HIG-level, app-author rules)

This section is the *application-author-facing* companion to Pulse Accessibility Rules (the platform-level constraints document); it tells app developers what's expected of them specifically, in HIG terms.

- Every custom control built by a third-party app must be reachable and operable via Tab/Shift+Tab and Enter/Space, with a visible focus ring meeting the contrast requirements in Accessibility Rules §3 — apps using only first-party Component Library controls get this for free; apps building bespoke controls must replicate it explicitly, and Pulse Store review checks for it.
- Apps must declare accessible labels for icon-only controls (no unlabeled icon buttons) — this is a Store submission requirement, not a suggestion.
- Apps must respect the system Reduce Motion and Reduce Transparency settings for any animation or blur they implement themselves, not just first-party chrome — an app that keeps animating heavily under Reduce Motion is treated as a Store policy violation.
- Color must never be an app's only signal for state (errors, success, required fields) — same rule as Accessibility Rules §3, restated here because it's the single most common third-party violation in comparable platforms' app stores.

---

## 10. Responsive layouts

### 10.1 Breakpoint philosophy
PulseLinux does not target phone-class breakpoints — this is a desktop/laptop/tablet OS. Three reference widths govern layout behavior: **Compact** (≤ 900px logical width — small tablets, narrow snapped windows), **Standard** (900–1440px — typical laptop), **Wide** (1440px+ — large monitors, ultrawide).

### 10.2 Behavior per breakpoint
- **Compact:** sidebars in apps (§2.1) collapse to an icon-only rail or a slide-over drawer triggered by a menu control; settings rows stack description text below the label instead of beside the control if needed.
- **Standard:** the baseline design target for every mockup in this system — assume Standard unless stated otherwise.
- **Wide:** content area gets a max-width constraint (apps don't stretch text columns edge-to-edge across an ultrawide monitor); sidebars may gain a secondary detail pane (e.g., Explorer gaining a preview pane) rather than just stretching existing elements wider, since empty stretched whitespace reads as unfinished, not premium.

### 10.3 Snapped windows
Any first-party app must remain fully usable snapped to a quarter-tile on a Standard display (roughly Compact width) — this is the actual real-world trigger for Compact layouts far more often than tablet hardware, and is treated as such in QA.

---


## 11. Keyboard shortcuts

### 11.1 Modifier key philosophy
PulseLinux uses **Super** (the Windows/Meta key) as its primary system modifier, not a new bespoke key combo — direct continuity with Principle 1. Super alone opens the Start Menu; Super + letter/number is the namespace for all system-level shortcuts.

### 11.2 Core system shortcuts (fixed, not remappable in their primary slot)
| Shortcut | Action |
|---|---|
| `Super` | Open/close Start Menu |
| `Super + S` | System-wide search |
| `Super + E` | Open Pulse Explorer |
| `Super + I` | Open Pulse Settings |
| `Super + A` | Open Quick Settings |
| `Super + N` | Open Notification Center |
| `Super + Tab` | Workspace Overview |
| `Super + ←/→` | Switch virtual desktop |
| `Super + ↑/↓/←/→` (window focused) | Snap window (maximize / minimize / left-half / right-half) |
| `Super + L` | Lock screen |
| `Super + Shift + S` | Pulse Screenshot (region capture) |
| `Super + .` | Clipboard History |
| `Alt + F4` | Close window |
| `Alt + Tab` | Switch window (within current desktop) |
| `Ctrl + Alt + Esc` | Pulse Task Manager |

This table is intentionally short. A platform that ships 80 default global shortcuts trains nobody — these eleven are chosen because they map directly to the most common Windows-honed muscle memory plus the handful of genuinely new Pulse-specific surfaces (Quick Settings, Clipboard History) that need a memorable, advertised entry point.

### 11.3 In-app conventions
Apps must honor standard conventions rather than reinvent them: `Ctrl+S` save, `Ctrl+Z`/`Ctrl+Shift+Z` undo/redo, `Ctrl+F` find, `Ctrl+,` (comma) open the app's own preferences, `Esc` closes the topmost transient surface (dialog, menu, sheet) before doing anything else. This consistency is a Store review criterion, not a suggestion.

### 11.4 Discoverability
Every menu item and titlebar action that has a shortcut displays it right-aligned within the menu row, in `--font-mono` at reduced size — discoverability through normal use, not through a separate "cheat sheet" the user has to go find. A dedicated, searchable shortcut reference still lives in Pulse Settings → Accessibility for users who want the full list at once.

---

## 12. Multi-monitor behavior

### 12.1 Taskbar
Default: the taskbar appears on the primary display only, matching most users' actual preference and avoiding the visual noise of duplicated chrome on every panel — but is independently configurable per-display in Settings for users who want it everywhere (a deliberate Windows-parity option, since "show taskbar on all displays" is a well-known preference for a meaningful minority of multi-monitor users).

### 12.2 Window and workspace behavior
- Virtual desktops are **per-system**, not per-monitor, by default (switching desktops moves all monitors together) — matching the simpler, more broadly-expected behavior — with a Settings toggle for power users who want independent per-monitor desktops, since both conventions exist across real-world Linux DEs and forcing one without an escape hatch is a common point of friction for advanced users migrating in.
- Windows remember which display they were on across sleep/wake and reconnect/disconnect events whenever the same display configuration is detected (matched by EDID), falling back to a sensible cascade onto the primary display when it isn't.
- Dragging a window to a screen edge that borders another display crosses over only past a deliberate resistance threshold (a brief pause at the edge) before transferring — preventing accidental hand-off during ordinary edge-snapping (§1.3) on the same display.

### 12.3 Mixed DPI and refresh rate
Each display renders at its own correct scale factor independently (no forced matching across displays); cursor and window content must not visibly "jump" size when crossing a display boundary mid-drag — content scales progressively across the boundary rather than snapping. Each display independently negotiates its own best supported refresh rate; the compositor does not force a lowest-common-denominator rate across all displays just because one is older.

---

## 13. OLED and HDR recommendations

### 13.1 OLED-specific guidance
- True black (`--pulse-void`, #050505) is used deliberately and extensively, per the theme spec — on OLED this isn't just an aesthetic choice but a genuine power-saving and burn-in-risk reduction strategy, and this document treats it as such: large static bright elements (a permanently-visible bright white panel, a static high-contrast logo) should be avoided in always-on contexts (e.g., a persistent always-on-display clock) in favor of dim, slowly-shifting-position renders.
- The taskbar and any persistent overlay must support a subtle pixel-shift / position-jitter strategy (a few px, imperceptibly slow, automatically applied after a configurable idle period) as a burn-in mitigation for displays detected as OLED — this is system-level compositor behavior, not something individual apps implement.
- Pure white (`#FFFFFF`) text or fills are avoided platform-wide in favor of the warm off-white `--pulse-text-primary` token, which is both a brand-consistency choice (per the theme spec) and a minor OLED-longevity benefit (slightly reduced peak subpixel drive versus true white at the same perceived brightness).

### 13.2 HDR
- On HDR-capable displays, `--pulse-signal` gains an optional HDR-boosted variant (modestly higher peak luminance, not a different hue) used **only** for the same small set of contexts the Beat already governs — Ambient/Response glow peaks — never applied to large static surfaces, so HDR headroom is used the same disciplined way pulse amplitude already is (Pulse Motion §2.3): a brief, earned moment of brightness, not a sustained one.
- SDR content (the vast majority of the desktop UI, all text, all standard surfaces) renders at standard SDR luminance even on an HDR-enabled display/output — PulseLinux does not "boost" the whole desktop just because HDR is available, which is a common and disorienting overcorrection on platforms that handle HDR poorly. HDR headroom is reserved for genuine HDR media content and the specific accent-glow exception above.

---

## 14. Application design rules (for first-party and Store apps)

### 14.1 Mandatory
- Use Component Library controls for all standard UI (buttons, toggles, inputs, menus, dialogs) rather than recreating them — visual or behavioral drift here is the single fastest way the platform stops feeling coherent, and is the top Store review rejection reason by design.
- Respect the sidebar + content navigation pattern (§2.1) for any app with more than one logical section.
- Implement Reduce Motion and Reduce Transparency fallbacks (Accessibility Rules §2, §6) for any custom animation or blur the app introduces beyond first-party components.
- Ship a recognizable app icon following Icon System §5's fixed badge shape — apps that ignore the badge shape are rejected at Store submission, not just stylistically flagged.

### 14.2 Strongly recommended
- Default to the platform's light-touch sound behavior (Sound Design §3) — silent by default for routine interaction, reserving sound for genuinely attention-worthy moments, with an opt-in rather than opt-out posture for anything louder.
- Prefer the three-tier notification priority system (§6.3) rather than inventing app-specific urgency levels.
- Adopt the Pulse type pairing (Inter Tight for chrome, Inter for body, JetBrains Mono for data/code) rather than introducing a third typeface family, which is permitted only with strong, documented justification (e.g., a code editor's necessary support for a specific ligature-rich programming font) and Store review sign-off.

### 14.3 Explicitly prohibited
- No custom window chrome that replaces the standard titlebar/control layout (§1.1–1.2) — even for "branding" reasons; this is the single rule most likely to be requested as an exception by an eager third-party developer, and the answer is no, because window-control consistency is foundational to Principle 1 (a user should always know where minimize/maximize/close are, in every app, without looking).
- No app-initiated system-wide modals that block interaction with other windows — an app may only ever modal-block its own window tree.
- No custom notification UI bypassing the system Notification Center / toast system — notifications are a platform service, not something an app renders itself.
- No use of `--pulse-signal` as a generic "branding" color inside a third-party app's own interface — Signal is reserved system-wide for the platform's own identity (Icon System §5), so it remains a reliable visual marker of "this is a system-level / first-party surface" even inside a busy Store full of third-party apps.

---
