# Pulse Animation Guidelines

**How to apply Pulse Motion correctly when building anything new. If Pulse Motion (doc 01) is the physics, this is the rulebook for engineers and designers using it.**

---

## 1. The one question to ask before animating anything

> "Is this thing alive on its own, or is it reacting to me?"

- **Alive on its own** → Ambient tempo, loops, only while the underlying state is true (charging, syncing, focused, loading). Stops the instant the state ends — never fades out gracefully "for style," it just stops, because the thing is no longer alive in that way.
- **Reacting to me** → Response tempo, plays exactly once, never loops.

If you can't answer this question about the thing you're animating, it probably shouldn't use the Pulse Beat at all — see §5.

---

## 2. Hierarchy of motion — only one thing pulses at a time, per region

A screen may have multiple Ambient pulses running simultaneously in different regions (the taskbar clock isn't pulsing while a download pulses in the tray — that's fine, they're unrelated). But **within a single component or contained region, never run two independent pulses at once.** A card that's both loading and has a charging-battery icon inside it should not show two beats fighting for attention. Pick the one that represents the most relevant state to the user's current task and silence the other (the other can still be true, just rendered statically until it becomes the relevant one).

This is the guideline most likely to get violated by enthusiastic contributors adding "just one more nice touch." Reviewers should treat a second simultaneous pulse in one region as a design bug, not a style nitpick.

---

## 3. Duration and tempo rules

- Never introduce a third tempo. If a new feature seems to need something between 220ms and 2400ms, it's almost always actually a Response beat (220ms) on a slightly larger element, not a new tempo. Slow it down by changing what's visible, not by changing the clock.
- Response beats never repeat automatically. If a user needs to be told "still working" after a Response beat plays, that's a transition into an Ambient state (e.g., button press → Response beat → button enters a loading Ambient state), not a repeating Response.
- Stagger, don't synchronize, when multiple independent elements share Ambient motion (e.g., several syncing files in a list). Each instance starts its own 2400ms cycle from when *it* began syncing, not from a shared global clock. Synchronized pulsing across unrelated items reads as a UI trick, not as several independent things being alive.

---

## 4. Amplitude discipline

- Default amplitude (§2.3 of Pulse Motion) is a ceiling, not a target. Many contexts should sit well under it — a tray icon's charging glow might move ±4% opacity, not ±18%. Reserve the top of the amplitude range for the few moments that should command real attention (primary button press, notification arrival).
- Never increase amplitude to compensate for a low-contrast design elsewhere. If a state isn't noticeable enough, fix its color or position — don't make it pulse harder.
- Test every new Ambient pulse with peripheral vision, not direct focus: glance away from the screen, then look back after one full cycle. If it looks like something is *flashing* rather than *breathing*, the amplitude or rise curve is wrong — go back to Pulse Motion §2 rather than hand-tuning a one-off easing curve.

---

## 5. When NOT to use the Beat

Motion that is not the Pulse Beat is allowed — Pulse Motion governs the "alive" signature, not every pixel of motion in the OS. Standard, non-Beat motion (simple linear or ease-out fades, slides, and dissolves) is used for:

- Window open/close/minimize transitions
- Menu and popover open/dismiss
- Page/panel transitions within an app
- Scroll-driven motion
- Drag and drop
- Hover states (per Pulse Motion §4, hover never uses the Beat)

These should be quick (120–200ms), use simple ease-out curves, and stay visually neutral — they are the OS's plumbing, not its identity. Mixing Beat-style heartbeat easing into routine UI plumbing (e.g., making every menu open with a heartbeat curve) overuses the signature until it stops meaning anything. The Beat is special *because* most motion in the OS is ordinary.

---

## 6. Composing the Beat with other motion

Sometimes a Response beat needs to coexist with an ordinary transition — e.g., a notification both slides in (ordinary motion) and plays a Response beat on its leading edge (Pulse Motion). Rule: ordinary motion handles **position and visibility**; the Beat handles **emphasis** layered on top, on a sub-element (an edge, an icon, a glow), never on the whole moving object at once. Do not apply the heartbeat curve to an entire object's entrance transition — that's how the Beat quietly turns into "everything bounces."

---

## 7. Review checklist for any new animated component

Before merging, a new animated component should be able to answer yes to all of the following:

1. Does it use exactly one tempo (Ambient or Response), not a custom duration?
2. Does it use the standard rise/hold/decay/rest proportions, not a custom easing curve?
3. Does it animate at most two properties simultaneously?
4. Does it stop immediately when its underlying state ends, with no embellished exit?
5. If it's an Ambient pulse, is it the *only* pulse active in its region?
6. Would it still look calm if viewed in peripheral vision for one full cycle?
7. Is its color limited to the Ember → Signal range, never brighter or hotter?

Any "no" sends it back to Pulse Motion, not to a new bespoke animation.
