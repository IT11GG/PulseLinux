# Pulse Theme Engine

**Layer:** Pulse API
**Depends on:** Pulse IPC, Pulse Service Manager, Pulse Settings API, Pulse Compositor (reads OLED/HDR output capability)
**Depended on by:** PulseDE shell, Pulse Application SDK, every visual component

---

## 1. Purpose

Pulse Theme Engine is the single authority for resolving design tokens (color, type, radius, motion timing — defined in the design system documents: theme spec, Pulse Motion, Component Library) into the actual values a given surface should render with right now, accounting for live accessibility settings (Reduce Motion, Reduce Transparency, High Contrast) and live display capability (OLED, HDR — read from Pulse Compositor's `OutputInfo`). No component reads `--pulse-*` CSS variables and separately checks accessibility settings itself; Theme Engine is the one place those two concerns (the design tokens, and the conditions that modify them) are combined.

This formalizes what the original prototype did with a single `data-reduce-motion` attribute toggle wired directly into the Settings app's onChange handler — functionally similar in outcome, but now expressed as a real subsystem with its own contract, reachable by any future consumer (a native compositor's animation pacing, a future High Contrast implementation) without re-deriving the wiring from scratch.

---

## 2. Core concepts

- **Token set** — the full named token table (colors, type, radius, motion durations/curves) as defined in the design system documents — Theme Engine does not invent new tokens, it resolves and serves the ones already specified.
- **Accessibility overrides** — Reduce Motion, Reduce Transparency, High Contrast (Pulse Accessibility Rules §2, §3, §6) are modeled as token *overrides* layered on top of the base token set, not as separate code paths components must branch on. A component asking Theme Engine "what's the Ambient beat duration right now" gets `0ms` directly when Reduce Motion is on — it never needs to know the override exists.
- **Output-aware resolution** — HDR-boosted Signal glow (HIG §13.2) and OLED burn-in pixel-shift eligibility (HIG §13.1) are resolved per-output, since a multi-monitor setup may have one HDR display and one SDR display simultaneously (HIG §12.3's mixed-capability handling carries directly into Theme Engine's resolution logic).

---

## 3. Interface contract

```ts
interface PulseThemeEngine {
  getToken(name: string): string | number;
  getTokenSet(): Record<string, string | number>;     // full resolved table, accessibility overrides already applied
  isReducedMotion(): boolean;
  isReducedTransparency(): boolean;
  isHighContrast(): boolean;
  resolveForOutput(outputId: string): OutputThemeContext;
}

interface OutputThemeContext {
  hdrSignalBoost: boolean;     // HIG §13.2 — only true on HDR-capable output AND not under any accessibility override that would conflict
  oledBurnInMitigation: boolean; // HIG §13.1
}
```

### 3.1 IPC surface
Registered as `pulse.theme`. Methods: `getToken`, `getTokenSet`, `isReducedMotion`, `isReducedTransparency`, `isHighContrast`, `resolveForOutput`. Signal: `theme:tokens-changed` `{ changedKeys }` — emitted whenever an accessibility setting changes and the resolved token set is affected, so any subscribed renderer (PulseDE shell, an app using the SDK) knows to re-apply.

---

## 4. Relationship to Pulse Settings API

Theme Engine does not own Reduce Motion/Reduce Transparency/High Contrast as its own state — it declares and reads them as Settings API keys (`display.reduceMotion`, `display.reduceTransparency`, `display.highContrast`, per Pulse Settings API §4) and subscribes to `settings:changed` for exactly those keys. This is the architectural point made in Pulse Settings API §4: Pulse Settings (the app) needs no theme-specific logic, because flipping a generic toggle in Settings *is* changing Theme Engine's input, mediated entirely through Pulse IPC signals rather than a direct function call between the two subsystems.

---

## 5. Relationship to PulseDE rendering

In the browser-prototype reference implementation, "applying" the resolved token set means setting `data-reduce-motion`/`data-reduce-transparency` attributes on `<html>`, exactly as the original prototype did — except now that write happens inside Theme Engine's own `settings:changed` listener, not inside the Settings app's button handler. PulseDE's shell calls `PulseThemeEngine.getTokenSet()` once at startup to know the current state, then simply trusts `theme:tokens-changed` signals thereafter rather than polling. A native compositor-level build would replace the DOM-attribute application step with whatever native theming/animation-pacing mechanism it uses, without changing this contract.

---

## 6. Why this is a separate subsystem from Pulse Settings API

It would be possible to let every component read Settings API's `display.reduceMotion` directly. Theme Engine exists as a distinct layer anyway for two reasons: first, token *resolution* (combining base tokens with overrides, output-aware HDR/OLED logic) is meaningfully more than a key lookup and deserves its own tested contract rather than being reimplemented at each call site; second, a future native compositor needs a single place to ask "what should I actually render right now" that isn't coupled to the settings storage mechanism underneath it — exactly the same separation-of-concerns argument made for Window Manager vs. Compositor (§4 of that document).
