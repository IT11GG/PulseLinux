# Pulse Window Manager

**Layer:** Pulse Runtime
**Depends on:** Pulse IPC, Pulse Compositor
**Depended on by:** Pulse Application SDK, PulseDE shell

---

## 1. Purpose

Pulse Window Manager owns everything the Compositor explicitly does not: what a "window" is (a surface plus metadata — title, icon, app identity, focus/minimize/maximize/snap state), focus policy, virtual desktops, and snap-layout geometry. This is the direct implementation of Pulse HIG §1 (Window behavior) and §1.5 (Multitasking) as a real service rather than a UI-layer convention.

Every window-related rule from the HIG is enforced here, centrally, rather than re-implemented per app: only one window may be focused at a time (HIG §1.2 carries directly into this layer's invariants), snap-zone geometry (HIG §1.3) is computed here so every app gets identical snapping behavior for free, and virtual desktop membership (HIG §1.5) is tracked here rather than by individual windows.

---

## 2. Core concepts

- **Window** — the policy-level object: id, owning app id, title, icon, geometry, focus/minimized/maximized/snapped state, workspace membership. Backed by exactly one Compositor surface (§3, Pulse Compositor) that Window Manager creates and updates on the window's behalf — apps never talk to the Compositor directly.
- **Focus** — exactly one window is focused at a time, system-wide (not per-output) per HIG §1.2; changing focus is the trigger for the Pulse Beat's focused-only hairline (Component Library §6), which Window Manager signals so PulseDE's rendering layer can react, without Window Manager itself knowing anything about the Beat.
- **Workspace** — a named virtual desktop (HIG §1.5); each window belongs to exactly one workspace at a time (or "all workspaces" for pinned windows, a future extension noted but not implemented in the prototype).
- **Snap zone** — a named geometry preset (`left-half`, `right-half`, `top-left-quarter`, etc., per HIG §1.3) Window Manager resolves against the current output's actual dimensions (queried from Compositor) so snapping is correct on any monitor size without the caller doing geometry math.

---

## 3. Interface contract

```ts
interface PulseWindowManager {
  open(request: OpenWindowRequest): WindowInfo;
  close(windowId: string): void;
  focus(windowId: string): void;
  minimize(windowId: string): void;
  toggleMaximize(windowId: string): void;
  snap(windowId: string, zone: SnapZone): void;
  move(windowId: string, x: number, y: number): void;
  resize(windowId: string, width: number, height: number): void;

  list(): WindowInfo[];
  get(windowId: string): WindowInfo | null;
  focused(): WindowInfo | null;

  // ---- Workspaces (HIG §1.5) ----
  listWorkspaces(): WorkspaceInfo[];
  createWorkspace(name: string): WorkspaceInfo;
  switchWorkspace(workspaceId: string): void;
  moveWindowToWorkspace(windowId: string, workspaceId: string): void;
  activeWorkspace(): string;
}

interface OpenWindowRequest {
  appId: string;
  title: string;
  icon?: string;
  width?: number; height?: number;
  x?: number; y?: number;
  singleInstance?: boolean; // default true — re-focuses existing window of same appId rather than opening a duplicate
}

interface WindowInfo {
  id: string; appId: string; title: string; icon?: string;
  x: number; y: number; width: number; height: number;
  z: number;
  focused: boolean; minimized: boolean; maximized: boolean;
  snapZone: SnapZone | null;
  workspaceId: string;
}

type SnapZone = 'left-half' | 'right-half' | 'maximize' |
  'top-left-quarter' | 'top-right-quarter' | 'bottom-left-quarter' | 'bottom-right-quarter';

interface WorkspaceInfo { id: string; name: string; }
```

### 3.1 IPC surface
Registered as `pulse.wm`. All interface methods above are exposed 1:1 as IPC methods. Signals: `window:opened`, `window:closed`, `window:focus-changed` `{ windowId }`, `window:minimized` `{ windowId }`, `window:maximize-toggled` `{ windowId, maximized }`, `window:snapped` `{ windowId, zone }`, `workspace:changed` `{ workspaceId }`.

---

## 4. Focus invariant

Window Manager enforces, as a hard invariant rather than a convention apps are trusted to respect: at most one window has `focused: true` at any time, and focusing a window always defocuses whatever was previously focused in the same operation (never two separate calls an app could get out of sync between). This is what lets PulseDE's rendering layer trust `window:focus-changed` as the single source of truth for which window should show the Beat's Ambient hairline, with no possibility of two windows briefly both appearing focused.

---

## 5. Snap geometry resolution

`snap(windowId, zone)` resolves the named zone against `PulseCompositor.getOutput()`'s real width/height (accounting for the output's reserved taskbar inset) rather than the caller supplying raw coordinates — this guarantees every app's windows snap identically and correctly regardless of display size, directly serving HIG §1.3's requirement that snap behavior be a consistent system service rather than something each app or window approximates.

---

## 6. Relationship to PulseDE and apps

PulseDE's shell and apps call Window Manager exclusively through Pulse IPC (`pulse.wm`) or the Application SDK's window helpers (§ Pulse Application SDK), which themselves call `pulse.wm`. No caller above this layer ever touches `pulse.compositor` directly, per Pulse Compositor §4's three-layer chain (`app -> Window Manager -> Compositor`).

---

## 7. What this replaces from the original prototype

`runtime/pulse-api.js`'s `WindowManagerService` directly created and mutated DOM-adjacent window state with no underlying Compositor concept — geometry, z-order, and "the actual pixels" were conflated into one object. This document's Window Manager owns only policy; actual surface creation/positioning is delegated to Pulse Compositor, restoring the split a real Wayland-backed window manager would have.
