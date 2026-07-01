# Pulse Application SDK

**Layer:** Pulse API → PulseDE boundary (this is what "Pulse Applications" in the project's layering diagram actually import)
**Depends on:** Pulse IPC, Pulse Window Manager, Pulse Filesystem API, Pulse Settings API, Pulse Theme Engine, Pulse Package API
**Depended on by:** every Pulse Application (Explorer, Settings, Store, and all future first- and third-party apps)

---

## 1. Purpose

The Application SDK is the one thing an app actually imports. Every other document in this set (IPC, Service Manager, Compositor, Window Manager, Session Manager, Filesystem API, Settings API, Package API, Theme Engine) is platform infrastructure an app should never need to know exists by name — the SDK wraps them into a single ergonomic, capability-scoped object, the same way a real application framework (e.g. GTK/Qt on Linux, or a mobile SDK) gives developers one cohesive API surface rather than requiring them to hand-roll D-Bus calls for every interaction.

This is the literal implementation of this round's stated goal: *"applications, PulseDE and system services communicate only through Pulse APIs."* An app built against the SDK cannot accidentally reach into Compositor internals or another app's memory — it only has the methods the SDK exposes, each of which is, underneath, a capability-scoped Pulse IPC call (Pulse IPC §5).

---

## 2. Core concepts

- **App context** — created once per app instance via `createPulseApp(manifest)`, returning a scoped SDK object bound to that app's declared capabilities (§3.2) and app id. An app never talks to a *shared, ambient* `PulseAPI` global (as the original prototype did) — it owns one context object, explicitly created, explicitly scoped.
- **Manifest** — minimal static declaration: app id, display name, icon, declared IPC capabilities (which endpoints/methods it's allowed to call — Pulse IPC §5), and window defaults. This is the architectural seed of a future real Pulse Store app manifest/package format.
- **Capability-scoped helpers** — the SDK's `window`, `fs`, `settings`, `pkg`, `theme` namespaces are thin, ergonomic wrappers around `PulseIPCConnection.call(...)` for that app's specific scoped connection — an app cannot use the SDK to call an endpoint or method outside its manifest's declared capabilities; doing so throws `CAPABILITY_DENIED` exactly as a raw out-of-scope IPC call would (Pulse IPC §5), just surfaced through a friendlier method name instead of a raw `call()`.

---

## 3. Interface contract

```ts
function createPulseApp(manifest: AppManifest): PulseAppContext;

interface AppManifest {
  appId: string;
  name: string;
  icon?: string;
  capabilities: string[];        // e.g. ['pulse.fs:*', 'pulse.notifications:*'] — Pulse IPC §5 format
  defaultWindow?: { width?: number; height?: number };
}

interface PulseAppContext {
  // ---- Identity ----
  appId: string;

  // ---- Window helpers (wraps pulse.wm) ----
  window: {
    open(opts: { title: string; icon?: string; content: Node }): Promise<WindowInfo>;
    close(windowId: string): Promise<void>;
    onFocusChanged(callback: (windowId: string) => void): Unsubscribe;
  };

  // ---- Filesystem helpers (wraps pulse.fs) ----
  fs: {
    list(path: string[]): Promise<Entry[]>;
    readFile(path: string[]): Promise<string>;
    writeFile(path: string[], content: string): Promise<void>;
    onChanged(callback: (path: string[]) => void): Unsubscribe;
  };

  // ---- Settings helpers (wraps pulse.settings, scoped) ----
  settings: {
    get(key: string): Promise<any>;
    set(key: string, value: any): Promise<void>;
    onChanged(key: string, callback: (value: any) => void): Unsubscribe;
  };

  // ---- Theme helpers (wraps pulse.theme — read-only for apps) ----
  theme: {
    getTokenSet(): Promise<Record<string, string | number>>;
    isReducedMotion(): Promise<boolean>;
    onTokensChanged(callback: () => void): Unsubscribe;
  };

  // ---- Package helpers (wraps pulse.pkg — typically Store-only capability) ----
  pkg: {
    search(query: string): Promise<PackageInfo[]>;
    install(packageId: string): Promise<void>;
  };

  // ---- Notifications (wraps pulse.notifications) ----
  notify(opts: { title: string; body: string; icon?: string; priority?: 'passive' | 'standard' | 'important' }): Promise<void>;
}
```

---

## 4. Why a manifest-scoped context object, not a global

The original prototype's apps (`apps/explorer.js`, `apps/settings.js`) imported a single shared `PulseAPI` singleton with full, unscoped access to every subsystem — convenient for a fast prototype, architecturally wrong for a platform meant to support third-party apps safely. `createPulseApp(manifest)` forces every app, including first-party ones, to declare up front what it needs, and the SDK enforces that declaration at the IPC layer rather than trusting app code to behave. Pulse Explorer's manifest declares broad `pulse.fs` access because it *is* the file manager (Pulse Filesystem API §4); a hypothetical third-party "PDF Reader" app's manifest would declare none of that, and the SDK object handed to it simply wouldn't have working `fs.list`/`fs.writeFile` calls beyond its narrow grant.

---

## 5. Relationship to Pulse Store and app installation

A real Pulse Store install (Pulse Package API §4) ships an app's manifest alongside its code; at launch, PulseDE's shell reads the manifest and calls `createPulseApp(manifest)` on the app's behalf before handing control to the app's own code — the app itself never constructs its own capability grant, since an app self-declaring its own permissions would defeat the sandboxing purpose entirely. This document specifies the SDK's shape; manifest verification/signing at install time is a Pulse Package API / Pulse Store concern layered on top, not duplicated here.

---

## 6. What this replaces from the original prototype

Every `import { PulseAPI } from '../runtime/pulse-api.js'` in `apps/explorer.js` and `apps/settings.js` is replaced by one `const app = createPulseApp(manifest)` call per app, with all subsequent calls going through `app.fs.*`, `app.settings.*`, etc. — same ergonomics from the app developer's point of view, fundamentally different security posture underneath, which is the whole point of formalizing this as its own platform layer rather than leaving it as an implicit convention.
