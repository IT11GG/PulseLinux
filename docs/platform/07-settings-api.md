# Pulse Settings API

**Layer:** Pulse API
**Depends on:** Pulse IPC, Pulse Service Manager
**Depended on by:** Pulse Settings (app), Pulse Theme Engine, Pulse Application SDK

---

## 1. Purpose

Pulse Settings API is the single persistent key-value store for system and per-app configuration. It replaces the original prototype's `SettingsService` with a schema-aware contract: keys are namespaced and declared with a type and default, so a caller can't silently read an undeclared key and get `undefined` with no indication of whether that means "off" or "doesn't exist" — a real source of bugs in loosely-typed settings stores.

Pulse Theme Engine (§ Pulse Theme Engine) is a first-class consumer of this API rather than maintaining separate state: Reduce Motion, Reduce Transparency, and accent-color preferences are Settings API keys the Theme Engine subscribes to, not theme-internal flags, so Pulse Settings (the app) can display and control them through the exact same generic settings-row UI as every other toggle, per Pulse HIG §5.

---

## 2. Core concepts

- **Namespace** — settings keys are dot-namespaced by owning domain (`network.wifi`, `display.reduceMotion`, `sound.systemSounds`), matching the original prototype's convention, now formalized with a registration step.
- **Schema entry** — each key is declared once (by the owning subsystem, at service start) with a type and default value, before it can be read or written — an undeclared key throws rather than silently returning `undefined`, surfacing a real bug (a typo'd key name, a missing registration) immediately instead of as a confusing "setting doesn't seem to do anything" report later.
- **Scope** — `system` (one value, machine/session-wide) or `per-app` (keyed additionally by app id — a future extension noted for app-specific preferences, e.g. an app's own window-remembering; not implemented in the prototype's reference, which only models `system` scope).

---

## 3. Interface contract

```ts
interface PulseSettingsAPI {
  defineKey(key: string, schema: KeySchema): void;
  get(key: string): any;
  set(key: string, value: any): void;
  reset(key: string): void;          // resets to the declared default
  resetNamespace(namespace: string): void; // HIG §5.4 — scoped reset
  list(namespace?: string): SettingEntry[];
}

interface KeySchema {
  type: 'boolean' | 'string' | 'number' | 'enum';
  default: any;
  enumValues?: string[]; // required if type === 'enum'
  label?: string;         // for generic settings-row rendering (HIG §5.2)
  description?: string;
}

interface SettingEntry {
  key: string;
  value: any;
  schema: KeySchema;
}
```

### 3.1 IPC surface
Registered as `pulse.settings`. All interface methods exposed 1:1. Signal: `settings:changed` `{ key, value, prevValue }`.

---

## 4. Relationship to Pulse Theme Engine and Accessibility Rules

Reduce Motion and Reduce Transparency (Pulse Accessibility Rules §2, §6) are defined as Settings API keys (`display.reduceMotion`, `display.reduceTransparency`) at Theme Engine's service start (§ Pulse Theme Engine §3). Theme Engine subscribes to `settings:changed` for those two keys specifically and re-applies the corresponding token overrides (the same `data-reduce-motion` attribute mechanism the original prototype used directly) whenever they change — meaning Pulse Settings (the app) needs no special-cased logic for these toggles; they render through the exact same generic `row()` helper as Wi-Fi or Night Light, and "flipping a toggle changes how the Beat behaves system-wide" emerges from the architecture rather than being hand-wired in the Settings app itself.

---

## 5. Why undeclared-key access throws

The original prototype's `SettingsService.get(key)` for an unregistered key silently returned `undefined`, which is indistinguishable from "this boolean setting is off." A schema-first contract — `defineKey` before any `get`/`set` — is a deliberate, small piece of platform discipline: it makes "this setting doesn't exist yet" a loud, immediate error at the call site that introduced the typo, rather than a silent, hard-to-diagnose false negative discovered later in a settings panel that mysteriously does nothing.

---

## 6. Honest scope of the reference implementation

Persistence is in-memory only (matching the rest of this prototype's runtime layer) — values reset on reload. The schema-registration and namespaced-reset mechanics are fully real, since those are architecture, not storage; only the "survives a restart" guarantee is deferred to a native build with real persistent storage underneath.
