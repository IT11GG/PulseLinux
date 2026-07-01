# Pulse Component Library

**Concrete component specifications. Every PulseLinux app and every PulseDE surface builds from these — not from one-off custom components.** Visual tokens (color/type/radius) are defined in `pulse-theme-spec`; this document defines structure and behavior, including exactly how (or whether) each component uses the Pulse Beat.

---

## 1. Component index

| Component | Uses the Beat? | Spec section |
|---|---|---|
| Button (standard) | Response, on press | §2 |
| Button (primary/destructive) | Response, on press (scale + glow) | §2 |
| Toggle switch | Response, on flip | §3 |
| Determinate progress bar | Never | §4 |
| Indeterminate loading bar | Ambient | §4 |
| Spinner | Ambient (replaces traditional spin where possible) | §5 |
| Window | Ambient on focused edge only | §6 |
| Notification | Response on arrival, then static | §7 |
| Volume indicator | Response on change | §8 |
| Battery / charging icon | Ambient while charging only | §9 |
| Lock screen | Ambient (signature edge) | §10 |
| Boot sequence | Ambient → single Response on completion | §11 |
| Text input | Hover/focus only, never Beat | §12 |
| Card / panel | Never, unless it contains a Beat-bearing child | §13 |

---

## 2. Button

**Anatomy:** label (Inter 500, 13px), optional leading icon, 9px/18px vertical/horizontal padding, `--radius-control` corners.

**Variants:**
- *Primary* — `--pulse-signal` fill, dark text. Reserved for the one clearly-intended action per view.
- *Secondary* — `--pulse-surface-2` fill, `--pulse-line` border.
- *Ghost* — transparent, text-only.
- *Destructive* — same structure as Primary, but the fill is `--pulse-ember` at rest (not Signal), shifting toward Signal only during its press Beat — this keeps destructive actions visually distinct from primary ones while sharing the family.

**Behavior:**
- Press: plays a single Response beat (Pulse Motion §2.2) — scale only for Secondary/Ghost; scale + glow for Primary/Destructive.
- Disabled: no Beat ever, 40% opacity, no hover response.
- Loading state (e.g., "Installing…"): label is replaced by a compact indeterminate bar (§4) inside the button; the button itself stops accepting press-Beats until loading ends.

---

## 3. Toggle switch

**Anatomy:** 40×24px pill track, 18px knob, 2px inset.

**Behavior:**
- Flip plays a Response beat: the knob's travel uses the Beat's rise/decay easing curves rather than a linear or generic ease-out slide — this is the one place a *position* change (not just opacity/scale) directly inherits the Beat's timing curve, because the toggle's entire job is one discrete state change, same as a heartbeat's single beat.
- Track color crossfades Surface-3 → Signal over the same 220ms, not a separate transition — track and knob settle together.
- No Ambient state. A toggle that's "on" is simply on; it doesn't breathe to remind you.

---

## 4. Progress bars

Two distinct components — never substitute one for the other:

**Determinate** (`<progress value>` semantics): a flat fill, width = real percentage, color `--pulse-signal`, 1px `--pulse-line` track. Updates only when real progress updates. No animation beyond a simple 150ms width transition on each update (ordinary motion, not the Beat — see Animation Guidelines §5).

**Indeterminate**: track is `--pulse-surface-2`; a soft 40%-width band of `--pulse-signal` brightness travels left-to-right on a 1.8s linear loop, while its *brightness* (not position) follows the Ambient Beat's rise/hold/decay/rest envelope. Position is mechanical and constant-speed; brightness is what's alive. This combination — steady mechanical travel, breathing brightness — is the bar's signature and should not be replicated elsewhere.

---

## 5. Spinner

PulseLinux avoids the traditional spinning-wheel spinner wherever a linear or radial alternative fits the space, because a spinning wheel has its own strong, unrelated visual identity (every OS has one, and it competes with the Beat rather than reinforcing it). Where a compact loading affordance is unavoidable (e.g., a small icon-sized slot), use a radial variant of the indeterminate bar: a thin ring where a Signal-colored arc breathes (Ambient envelope on the arc's opacity) while slowly rotating at constant speed — same logic as §4, applied radially.

---

## 6. Window

**Anatomy:** see `pulse-theme-spec` for surface/radius/titlebar tokens.

**Behavior:**
- Only the **focused** window's titlebar edge runs the Ambient Beat (the hairline signature). All other visible windows' edges are static and slightly dimmer (`--pulse-line` at 60% opacity instead of the animated gradient).
- On focus change: the newly-focused window plays one Response beat on its edge (a brief brighten-and-settle) before transitioning into its Ambient loop. The window losing focus does not play an exit animation on its edge — it simply stops, per Animation Guidelines §1.
- Open/close/minimize: ordinary motion only (ease-out scale + fade, ~180ms). The Beat is reserved for the focus signature, not the window's entrance.

---

## 7. Notification

**Anatomy:** `--pulse-surface-2` card, `--radius-window` corners, icon + title (Inter 500) + body (Inter 400, secondary text), floats top-right, glass blur consistent with taskbar/overlay treatment.

**Behavior:**
- Entrance: slides in with ordinary motion (position is ordinary motion per Animation Guidelines §6); simultaneously, its leading edge (a 2px accent strip) plays one Response beat as emphasis layered on top.
- Once settled, fully static — a notification does not breathe at you while it waits to be dismissed. Persistent ambient motion on something demanding attention reads as nagging, which the brief's "calm" requirement explicitly rules out.
- Dismiss: ordinary fade/slide out, no Beat.

---

## 8. Volume indicator

**Anatomy:** a row of discrete level pips (not a continuous slider) plus a percentage readout in `--font-mono`.

**Behavior:** on any volume change (hardware key, slider drag, app-triggered), the single pip nearest the new level plays a Response beat (brief scale + brightness). Pips do not animate in sequence/cascade — only the one nearest the current value, once. This keeps the affordance quick and legible rather than a swooshing light show.

---

## 9. Battery / charging icon

**Behavior:** while actually charging, the bolt glyph's glow runs the Ambient Beat at reduced amplitude (~±4% opacity, well under the general ceiling — see Animation Guidelines §4). The instant charging stops (unplugged, or reaches 100%), the glow simply stops — no fade-out flourish. A fully charged, plugged-in battery is static, not eternally pulsing; "alive" describes activity, not a permanent decoration.

---

## 10. Lock screen

**Anatomy:** full-bleed wallpaper, time in large Inter Tight, a single hairline near the bottom edge of the screen.

**Behavior:** that bottom hairline runs the exact same Ambient Beat animation as a focused window's titlebar edge — same duration, same curve, same color range. This is intentional repetition: the lock screen is, structurally, "the one thing that's alive" on an otherwise static screen, exactly like a focused window is the one alive window on an otherwise static desktop. Reusing the identical animation (not a "lock screen version" of it) is what makes the signature legible across completely different contexts.

---

## 11. Boot sequence

**Behavior:** the Pulse mark renders centered on `--pulse-void`, running the Ambient Beat (glow blur + opacity) for the duration of boot. The instant the system is ready to hand off to the login/lock screen, the mark plays exactly one Response beat — a slightly stronger, final "beat" — before crossfading to the lock screen. This single moment (Ambient loop → one final emphasized Response → handoff) is the closest thing PulseLinux has to a "brand sting," and it should never be elaborated beyond this — no sound swell, no logo animation beyond the Beat itself (sound is specified separately in Pulse Sound Design, and is intentionally minimal there too).

---

## 12. Text input

Focus state is a 3px signal-dim ring plus border color shift to `--pulse-signal`, both via a 100ms ordinary fade — never the Beat. Typing, validation errors, and placeholder states are all ordinary motion. Inputs are where users spend sustained attention; introducing ambient motion here would be actively distracting, not alive.

---

## 13. Card / panel (generic container)

A plain card never animates on its own. If a card contains a Beat-bearing element (a loading row, a charging device entry in Settings, a syncing file), only that child animates — the card itself, its border, and its shadow remain completely static. This containment rule is what keeps list-heavy surfaces (Settings, Pulse Explorer, Pulse Store) from turning into a wall of competing pulses — see Animation Guidelines §2.
