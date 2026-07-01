# Pulse Sound Design

**The sound language for PulseLinux. Sound is the rarest, most restrained layer of the Pulse identity — most interactions should be silent.**

---

## 1. Principle

If Pulse Motion is "alive, but calm," Pulse Sound is "present, but almost silent." The single biggest risk in OS sound design is over-scoring — a notification chime, a click sound, a connect sound, a disconnect sound, a low-battery sound, a screenshot shutter, an error buzz, a success chime, all competing for the same half-second of the user's attention many times an hour. PulseLinux's sound set is deliberately smaller than a typical OS's, not larger.

The test for any proposed sound: **would a user who's heard it 500 times still not find it annoying?** If a sound is even slightly grating on repetition, it doesn't ship, regardless of how good it sounds in isolation.

---

## 2. The sound palette

All Pulse sounds are built from one sound family — a soft, low-harmonic "tone" with a fast attack and a long, smooth decay, deliberately mirroring the Pulse Beat's rise/hold/decay/rest shape (Pulse Motion §2.1) so the audio identity and motion identity are the same gesture in two senses. No sound in the system uses a bright, percussive "click," "ding," or synthetic UI-blip register — these read as generic consumer software, not as a deliberate, premium identity.

There are exactly three tone variants, at three pitches, and nothing else:

| Tone | Used for | Pitch register |
|---|---|---|
| **Pulse Low** | System-level state changes: boot complete, shutdown, lock, unlock, charger connect/disconnect | Low-mid |
| **Pulse Mid** | Direct interaction feedback: notification arrival, volume change reaching min/max, screenshot capture | Mid |
| **Pulse High** | Rare "good news" confirmations: successful install/update complete, backup complete | Mid-high |

Each tone is a single note (not a chord progression, not a melodic phrase) lasting 180–320ms, matching Response-tempo timing from Pulse Motion. There is no "error" tone in this palette — see §4.

---

## 3. What is silent

By default, PulseLinux does **not** play sound for: routine button presses, toggle flips, window focus/open/close, typing, menu open/dismiss, hover, scrolling, or routine navigation. Silence is the default state of the OS; sound is reserved for moments that genuinely benefit from an audible confirmation, almost all of which involve the user's attention being elsewhere (charging cable across the room, install finishing in the background) rather than moments the user is already looking directly at.

This is a deliberate departure from typical desktop sound schemes (which often sound a tone for clicks, errors, and navigation) — those exist mostly as legacy/accessibility carryover, and PulseLinux instead routes most of that feedback through Accessibility-specific channels (see Pulse Accessibility Rules) rather than making sound a default layer for sighted, mouse-and-keyboard use.

---

## 4. Errors and warnings

Errors use **Pulse Low**, the same tone family as calm system events — not a distinct alarm register. PulseLinux does not have a separate "error buzz." This mirrors Pulse Motion §4's rule that errors get one calm beat rather than a panic cue: an error is communicated through what's on screen (color, copy, iconography) and a single low, calm tone, not through an audibly alarming sound. A user should never be startled by PulseLinux.

---

## 5. Spatial and material quality

All tones are produced with a soft low-pass character (no bright upper harmonics, no metallic ring) — closer to a soft mallet on a wood block than a glass or metal chime. This keeps the whole palette feeling warm and analog-adjacent, consistent with the warm-tinted blacks in the visual theme rather than the cold, glassy chimes typical of "futuristic" UI sound design.

---

## 6. Volume and mixing behavior

- All system tones play at a fixed, modest relative volume regardless of media volume level, and respect a dedicated "system sounds" volume control separate from media/app volume (in Settings, not buried in a mixer).
- Two tones never overlap. If a second sound-worthy event occurs while a tone is still playing (within ~320ms), the second event is queued silently and only shown visually — sound does not stack or cut itself off mid-decay, which is one of the most common ways an OS's sound design starts to feel chaotic under real multitasking conditions.
- Sound is fully muted automatically during Do Not Disturb / Focus modes, with zero exceptions, including the install/update completion tones — those instead surface silently as a Component Library notification once Focus mode ends.

---

## 7. What this system explicitly avoids

- A distinct sound for every discrete UI action (the "everything chimes" school of OS sound design) — this is the single most common mistake, and the brief's "calm, elegant, premium" requirement rules it out the same way it rules out flashy animation.
- Melodic "brand jingles" (a multi-note startup tune) — PulseLinux's boot sound, if used at all, is a single Pulse Low tone synced to the boot mark's final Response beat (Component Library §11), not a composed musical sting.
- Voice prompts or spoken feedback outside of Pulse AI's deliberate voice-support feature and dedicated accessibility screen-reader output.
