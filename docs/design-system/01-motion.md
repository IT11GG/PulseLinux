# Pulse Motion

**The motion specification for PulseLinux. Every animated thing in the OS derives from this document. Nothing animates outside of this system.**

---

## 1. The core idea

PulseLinux feels alive. Not energetic, not flashy — alive the way a resting body is alive: a slow, steady, involuntary rhythm running underneath everything, occasionally rising to meet a moment of attention, then settling back down.

This is the single idea every other Pulse spec inherits. If a future component, app, or contributor wants to add motion that doesn't trace back to the primitive defined below, it doesn't belong in PulseLinux.

The test for any new animation: **could you tell it was Pulse with the logo removed?** If the curve, timing, and restraint feel generic, it fails, regardless of how good it looks in isolation.

---

## 2. The Pulse Beat — the one primitive

There is exactly one motion primitive in PulseLinux. Everything else in this document is that primitive applied at different scales, durations, and properties.

### 2.1 Shape

The Pulse Beat is modeled on a resting heartbeat, not a UI bounce. A real heartbeat is not symmetrical and not bouncy — it has a fast, slightly sharp rise, a brief hold near the peak, and a slow, longer decay back to rest. That asymmetry is what makes it read as biological rather than mechanical. A symmetric ease-in-out (the thing most "breathing" UI animations actually use) reads as a machine oscillating, not something alive — this is the single most common way a Pulse-inspired knockoff will get it wrong, so it's worth naming precisely:

```
amplitude
   │
   │      ╭─╮
   │     ╱   ╲___
   │    ╱        ╲____
   │   ╱              ╲______________
   │  ╱                              ╲___________
   └──────────────────────────────────────────────→ time
      rise   hold   decay (long)         rest (long)
      18%    8%     34%                  40%
      of cycle
```

- **Rise — 18% of the cycle.** Curve: `cubic-bezier(0.25, 0.05, 0.35, 1)`. Fast but not instant; this is the only part of the beat that should ever feel sharp.
- **Hold — 8% of the cycle.** A near-imperceptible pause at peak amplitude. This is what separates "heartbeat" from "sine wave." Never skip this step even at small scale.
- **Decay — 34% of the cycle.** Curve: `cubic-bezier(0.65, 0, 0.35, 1)`. Long, soft, slightly slower than the rise. This is the part people unconsciously register as "calm."
- **Rest — 40% of the cycle.** Flat. Nothing happens. The rest period is not dead time to be trimmed — it is what makes the next beat feel like a pulse and not a flicker. Never reduce rest below 30% of the cycle to "fit more beats in."

### 2.2 The two tempos

The Beat exists at exactly two tempos. Do not invent a third.

| Tempo | Total cycle | Used for |
|---|---|---|
| **Ambient** | 2400ms | Idle/passive states: breathing indicators, charging, syncing, the lock screen edge, the boot mark |
| **Response** | 220ms | Reactions to direct input: button press, toggle flip, window focus change, notification arrival |

Response is not a sped-up Ambient with the rest period removed — it's the same proportional shape (18/8/34/40), just compressed to 220ms total, and it plays exactly once rather than looping. Ambient loops indefinitely while its state is active.

### 2.3 Amplitude is always small

The Beat moves one or more of: opacity (±0.08–0.18), scale (±1–3%), a glow's blur radius (±2–6px), or a 1–2px translation. It never combines more than two of these at once. A button that pulses on press should *not* scale, glow, and shift color simultaneously — pick the one or two properties that suit the context (see §3) and leave the rest static. Combining all available properties is the single fastest way to drift from "alive" into "RGB gamer," and is explicitly disallowed.

### 2.4 Color of the pulse

When the Beat drives a glow or color shift, it moves between `--pulse-ember` (rest) and `--pulse-signal` (peak) — never to a brighter red than Signal, never to white, never to a secondary hue. A pulse that flashes white-hot or cycles through colors is not Pulse motion; it's decoration wearing Pulse's name.

---

## 3. Where the Beat appears

Every one of these is the *same* curve and proportions from §2, scaled in duration and amplitude only. None of them get a bespoke animation curve.

| Surface | Tempo | Property animated | Notes |
|---|---|---|---|
| Boot mark | Ambient | glow blur + opacity | Loops until boot completes, then plays one final Response beat as the desktop appears |
| Lock screen edge | Ambient | the hairline signature (see Component Library §2) | Identical animation to the active-window titlebar edge |
| Active window titlebar edge | Ambient | hairline opacity + scaleX | Only the focused window's edge beats; unfocused windows are static |
| Window focus change | Response | the newly-focused window's edge plays one Response beat before settling into Ambient | Signals "this one is alive now" |
| Button press | Response | scale (99%→101%→100%) | No glow on standard buttons — see §4 exceptions |
| Primary/destructive button press | Response | scale + glow | The one case where two properties combine, because primary actions earn slightly more emphasis |
| Toggle switch | Response | the knob's travel uses the Beat's rise/decay timing, not a linear slide | |
| Progress bar (determinate) | N/A | fill advances linearly with real progress — see §4, progress bars do not pulse | |
| Progress bar (indeterminate / loading) | Ambient | a soft band of Signal brightness travels the bar's length, fading at the Beat's tempo | This is the one Ambient case that also has horizontal motion; the brightness fade still follows rise/hold/decay/rest |
| Notification arrival | Response | the notification's leading edge plays one Response beat as it enters | Same as window focus — "something just became alive" |
| Volume indicator (on change) | Response | the level pip nearest the new value plays one Response beat | |
| Battery charging | Ambient | the charge-bolt icon's glow | Stops the instant charging completes — fully charged is static, not a slow eternal pulse |
| Update available / installing | Ambient | the update icon in the tray | Switches to a Response beat once on completion, then returns to static |
| Cursor hover (interactive elements) | — | **no Beat.** Hover uses a simple 100ms linear fade, not the Beat. | The Beat is reserved for things that are "alive on their own," not for tracking the mouse |

---

## 4. Explicit exceptions — what never pulses

Naming these prevents future scope creep, which is the most likely way this system degrades over time:

- **Determinate progress bars never pulse.** They are an honest, literal representation of real progress. Pulsing a determinate bar lies about what's happening. Only *indeterminate* loading states use the Beat.
- **Text never pulses.** No breathing labels, no glowing headlines.
- **Hover states never use the Beat.** Reserved for system-driven "aliveness," not mouse-tracking feedback.
- **Backgrounds and wallpapers never pulse**, even ambient ones. The Beat is a foreground signature; if it's everywhere, it's nowhere.
- **Error states do not pulse red faster as a panic cue.** An error gets a single, calm Response beat on arrival, then goes static. PulseLinux does not flash at users.
- **Nothing pulses faster than the Response tempo (220ms) under any circumstance**, including "urgent" notifications. Urgency is communicated through color, iconography, and copy — never through speeding up the heartbeat. A faster pulse reads as panic, which directly contradicts "calm, elegant, premium."

---

## 5. Why this, not a generic breathing-glow system

Most "alive OS" concepts default to either (a) a slow sine-wave glow on everything, or (b) fast, springy, game-engine bounce easing borrowed from mobile app libraries. Both were considered and rejected:

- A symmetric sine breathing glow (the obvious default) reads as a screensaver, not a heartbeat — it has no rise/hold/decay asymmetry, so nothing about it feels biological. It also has no natural "response" mode, so input feedback ends up using a totally different curve, breaking the one-primitive rule immediately.
- Springy/bouncy easing (think default Framer Motion springs) is expressive and fun, which is exactly the "RGB gamer" / consumer-app energy the brief explicitly rejects. It also doesn't scale down to a 220ms button press without looking jittery.

The heartbeat-shaped asymmetric curve, restricted to two tempos and a hard amplitude ceiling, is the version of "alive" that stays calm under the brief's actual constraint: recognizable, premium, never flashy.

---

## 6. Reference implementation (CSS)

```css
:root {
  /* Pulse Beat — Ambient (2400ms total cycle) */
  --beat-ambient-duration: 2400ms;
  --beat-rise: 18%;
  --beat-hold: 8%;
  --beat-decay: 34%;
  --beat-rest: 40%;

  --ease-beat-rise: cubic-bezier(0.25, 0.05, 0.35, 1);
  --ease-beat-decay: cubic-bezier(0.65, 0, 0.35, 1);

  /* Pulse Beat — Response (220ms, single play) */
  --beat-response-duration: 220ms;
}

@keyframes pulse-beat-ambient {
  0%    { opacity: 0.45; }
  18%   { opacity: 0.95; }            /* end of rise */
  26%   { opacity: 0.95; }            /* hold */
  60%   { opacity: 0.45; }            /* end of decay */
  100%  { opacity: 0.45; }            /* rest */
}

@keyframes pulse-beat-response {
  0%    { transform: scale(1);    }
  18%   { transform: scale(1.03); }
  26%   { transform: scale(1.03); }
  100%  { transform: scale(1);    }
}

.pulse-ambient {
  animation: pulse-beat-ambient var(--beat-ambient-duration) infinite;
  animation-timing-function: linear; /* easing lives inside the keyframe steps above */
}

.pulse-response {
  animation: pulse-beat-response var(--beat-response-duration) 1;
}
```

> Implementation note for PulseDE (Wayland compositor): the reference above is CSS for app-level/Settings/Store contexts. The compositor itself implements the identical rise/hold/decay/rest proportions and easing curves natively in its animation engine (§ see PulseDE Animation Engine spec) so window-manager-level effects — focus change, workspace switch — share the exact same physical feel as in-app motion. The curve is the contract; the renderer is implementation detail.
