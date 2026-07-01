# PulseLinux Platform Architecture — Index

**The ten Pulse platform subsystems, how they layer, and how this maps onto the project's long-term Kernel-independence goal.**

---

## 1. The layering, restated against real modules

```
Kernel                         (not modeled in this prototype — real Linux kernel in a native build)
   ↓
System Services                Pulse IPC, Pulse Service Manager
   ↓
Pulse Runtime                  Pulse Compositor, Pulse Window Manager, Pulse Session Manager
   ↓
Pulse API                      Pulse Filesystem API, Pulse Settings API, Pulse Package API, Pulse Theme Engine
   ↓
PulseDE                        the shell (boot/lock/login/desktop, taskbar, start menu, etc.)
   ↓
Pulse Applications              Pulse Explorer, Pulse Settings, Pulse Store, and all future apps —
                                 built exclusively against the Pulse Application SDK
```

Every arrow above is a real, enforced dependency in code, not just a diagram: each module in this prototype only imports from the layer(s) directly beneath it, and cross-subsystem communication at the same layer (e.g. Theme Engine reacting to Settings changes) happens over Pulse IPC signals, never direct method calls into another subsystem's internals.

---

## 2. Reading order

| # | Document | What it owns |
|---|---|---|
| 1 | `01-pulse-ipc.md` | The message bus everything else rides on — method calls, signals, capability scoping |
| 2 | `02-pulse-service-manager.md` | Service lifecycle, dependency-ordered startup, restart policy |
| 3 | `03-pulse-compositor.md` | Surfaces, outputs, frame pacing — the contract beneath window management |
| 4 | `04-pulse-window-manager.md` | Window policy: focus, snapping, virtual desktops — built on the Compositor |
| 5 | `05-pulse-session-manager.md` | The boot → lock → login → desktop lifecycle, gated on real service readiness |
| 6 | `06-pulse-filesystem-api.md` | File access, the sandbox enforcement point for app file scope |
| 7 | `07-pulse-settings-api.md` | Schema-first persistent key-value store |
| 8 | `08-pulse-package-api.md` | PulsePkg's programmatic surface — install/remove/update/rollback |
| 9 | `09-pulse-theme-engine.md` | Token resolution + live accessibility overrides, output-aware HDR/OLED |
| 10 | `10-pulse-application-sdk.md` | The one thing an app actually imports — composes 1–9 into one capability-scoped surface |

Documents 1–2 are foundation (everything depends on them). 3–4 are a pair (Compositor is the primitive, Window Manager is the policy on top). 5 depends on 2 and indirectly on everything it gates. 6–9 are independent peers, each a Pulse API subsystem in its own right. 10 depends on all of 1, 4, 6, 7, 8, 9 and is the only one apps should ever import directly.

---

## 3. What changed from the previous prototype round

The previous round (`runtime/pulse-api.js`) had every subsystem as a method on one shared, unscoped, frozen object, instantiated in source order with no formal service lifecycle, no capability boundaries, and direct method calls between subsystems. This round replaces that with: a real message-passing layer apps and services are equally subject to (Pulse IPC), explicit dependency-ordered service bring-up instead of import-order coincidence (Service Manager + Session Manager), a genuine Compositor/Window-Manager split mirroring real Wayland architecture, schema-enforced settings instead of silent `undefined`, and a capability-scoped Application SDK so "an app can only do what its manifest says" is an enforced property, not a convention.

This is the necessary platform substrate the project's original long-term goal depends on: *"replacing the Linux kernel in the future would not require redesigning the desktop, applications, or user experience."* Everything above Pulse IPC now only knows about Pulse interfaces — swapping what sits underneath Pulse IPC's transport, or what a service's `start()` actually does internally, is the only thing a future native/kernel-level port needs to touch.

---

## 4. Verification

All ten subsystems are exercised by `smoke-test.mjs` (43 passing assertions covering IPC method/signal semantics, capability denial, dependency-ordered service startup, the Session Manager's legal-transition enforcement and service-readiness gating, the Window Manager's focus invariant and output-aware snap geometry, Filesystem CRUD, Settings schema enforcement, Theme Engine reacting to Settings purely over IPC, Package API's async operation + signal pattern, and SDK capability scoping). Run with `node smoke-test.mjs` from the project root.

---

## 5. What is still a stub, honestly

Per each document's own "honest scope" section: the Compositor models one output (no real multi-monitor/HDR/OLED detection), the Filesystem API is in-memory with no real disk I/O, Settings persistence does not survive a reload, the Package API simulates network/install timing against a four-item fixed catalog with no real signature verification, and IPC's capability enforcement is real at the contract level but does not yet provide true OS-level process isolation. None of these gaps require a contract change to fix later — each is exactly the kind of "swap the implementation behind the interface" work the layering was built to make possible.
