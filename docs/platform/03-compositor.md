# Pulse Compositor (abstraction layer)

**Layer:** Pulse Runtime
**Depends on:** Pulse IPC, Pulse Service Manager
**Depended on by:** Pulse Window Manager

---

## 1. Purpose

The Compositor abstraction is the lowest-level visual primitive PulseLinux exposes above the kernel/graphics-stack: it owns *surfaces* (a rectangular pixel buffer with a position, size, and stacking order) and *outputs* (displays), and nothing else. It does not know what a "window" is, does not know about titlebars, does not know about focus policy, does not know about snapping — all of that is Window Manager policy, layered on top (§ Pulse Window Manager). This split mirrors the real Wayland architecture deliberately: a Wayland compositor (e.g. wlroots) provides exactly this same minimal surface/output primitive, and a separate window-manager component (the actual "DE" logic) implements policy on top of it. Keeping that split honest now, even in a browser prototype where DOM elements stand in for real GPU buffers, is what makes a future swap to a real wlroots-based native compositor a backend substitution rather than an architecture change.

---

## 2. Core concepts

- **Surface** — a rectangular content buffer: id, size, position, z-order, and a content handle (in this prototype, a DOM node; in a native build, a GPU-backed buffer/texture). The Compositor does not interpret surface content.
- **Output** — a display: id, resolution, scale factor, refresh rate, position in the global output layout (for multi-monitor — HIG §12). The prototype models exactly one output (the browser viewport) but the interface supports N.
- **Damage region** — the sub-rectangle of a surface that changed since last composite, used to avoid redrawing unchanged pixels. The prototype's reference implementation tracks this for contract-completeness even though the browser's own renderer makes it largely redundant today — a native build's actual performance depends on this being real.
- **Frame callback** — a per-output vsync-aligned callback surfaces can request, used to pace animation (including the Pulse Beat) to the display's actual refresh rate rather than a fixed JS timer.

---

## 3. Interface contract

```ts
interface PulseCompositor {
  // ---- Surfaces ----
  createSurface(options: SurfaceOptions): SurfaceHandle;
  destroySurface(id: string): void;
  updateSurface(id: string, patch: Partial<SurfaceOptions>): void;
  setStackOrder(id: string, z: number): void;

  // ---- Outputs ----
  listOutputs(): OutputInfo[];
  getOutput(id: string): OutputInfo;

  // ---- Frame pacing ----
  requestFrame(outputId: string, callback: (frameTime: number) => void): void;

  // ---- Damage tracking ----
  damage(surfaceId: string, region?: Rect): void;
}

interface SurfaceOptions {
  x: number; y: number; width: number; height: number;
  outputId?: string;       // which output this surface is primarily on
  content: Node | unknown; // DOM node in this prototype; opaque buffer handle in a native build
}

interface SurfaceHandle {
  id: string;
}

interface OutputInfo {
  id: string;
  width: number; height: number;
  scaleFactor: number;
  refreshRate: number;
  x: number; y: number;       // position within the global output layout — HIG §12
  isPrimary: boolean;
  isHDRCapable: boolean;       // HIG §13.2
  isOLED: boolean;              // HIG §13.1 — drives burn-in mitigation eligibility
}

interface Rect { x: number; y: number; width: number; height: number; }
```

### 3.1 IPC surface
Registered as `pulse.compositor`. Methods: `createSurface`, `destroySurface`, `updateSurface`, `setStackOrder`, `listOutputs`, `getOutput`, `damage`. Signals: `output:added`, `output:removed`, `output:changed` (resolution/scale/refresh changes — relevant to HIG §12.3 mixed-DPI behavior), `surface:destroyed`.

---

## 4. Relationship to Pulse Window Manager

The Compositor never makes policy decisions — it does not decide which surface should be focused, does not decide snapping geometry, does not know what "maximized" means. Pulse Window Manager (next document) is the sole caller of this interface for window-related surfaces, translating user/app intent ("open this app's window", "snap left", "focus this") into the low-level `createSurface`/`updateSurface`/`setStackOrder` calls. PulseDE's shell and apps never call the Compositor directly — they go through Window Manager, which goes through Compositor. This three-layer chain (`app -> Window Manager -> Compositor`) is intentional and mirrors `app -> DE -> Wayland compositor` in a real Linux desktop stack.

---

## 5. OLED / HDR awareness (HIG §13)

`OutputInfo.isOLED` and `isHDRCapable` exist at this layer, not higher, because display capability detection is fundamentally a compositor/output-layer concern — the same place a real Wayland compositor would query EDID/DRM properties. Window Manager and Theme Engine read these flags (e.g., to drive pixel-shift burn-in mitigation or HDR-boosted Signal glow per HIG §13) rather than re-deriving them, keeping display-capability knowledge in exactly one place.

---

## 6. Honest scope of the browser-prototype reference implementation

The reference implementation in `compositor/pulse-compositor.js` models surfaces as DOM elements positioned absolutely within a single root container, and treats the browser viewport as the one `OutputInfo`. Damage tracking and frame-callback pacing are implemented against `requestAnimationFrame` rather than a real vsync signal. This is an honest stand-in for the contract, not a complete graphics stack — the documented interface, not this specific implementation, is the thing a native build commits to preserving.
