# Pulse IPC

**Layer:** System Services (foundation layer all other Pulse APIs ride on)
**Depends on:** nothing
**Depended on by:** every other Pulse subsystem in this document set

---

## 1. Purpose

Pulse IPC is the single communication substrate for PulseLinux. Every cross-boundary interaction in the system — an app asking the Window Manager to open a window, the Session Manager asking a service to start, PulseDE asking the Theme Engine to switch palettes — goes over Pulse IPC. No subsystem is permitted to call directly into another subsystem's internals or hold a live object reference to it; everything is mediated by this layer, the same way real desktop Linux mediates cross-process communication through D-Bus rather than processes reaching into each other's memory.

This is what makes the rest of the platform genuinely swappable later: as long as a message named `wm.openWindow` produces the contractually-described response, it does not matter whether the call crossed a JS module boundary in this prototype or a real process boundary over a Unix socket in a native build.

### 1.1 Why this exists as its own layer, named explicitly
The previous prototype (`runtime/pulse-api.js`) had services call each other's methods directly via a shared `PulseAPI` object and a single `EventTarget` bus. That's fine for a single-process browser demo, but it quietly assumes everything lives in one address space — an assumption a real OS cannot make (the compositor, the package manager, and an app are different processes, often different privilege levels). Naming the IPC layer explicitly, and routing *everything* through it rather than through direct method calls, is what makes the eventual native port a matter of swapping the transport, not redesigning the call graph.

---

## 2. Core concepts

- **Endpoint** — a named, addressable destination a service registers (e.g. `pulse.wm`, `pulse.session`, `pulse.fs`). Endpoint names are reverse-domain-free, flat, dot-namespaced strings.
- **Method call** — a request to a specific endpoint + method, with a payload, that returns a Promise resolving to a response or rejecting with a structured `PulseIPCError`. Mirrors a D-Bus method call / Wayland request.
- **Signal** — a fire-and-forget broadcast from an endpoint that any number of listeners can subscribe to, with no response expected. Mirrors a D-Bus signal / Wayland event.
- **Capability token** — an opaque token granted to a caller at connection time, scoping which endpoints/methods it's allowed to reach (see §5). This is where sandboxing (HIG §14, "application sandbox" in the original brief's security section) is anchored architecturally, even though the prototype's capability check is a stub.

---

## 3. Interface contract

```ts
interface PulseIPC {
  // ---- Endpoint registration (services only) ----
  registerEndpoint(name: string, handlers: Record<string, MethodHandler>): EndpointHandle;
  unregisterEndpoint(name: string): void;

  // ---- Method calls (callers: apps, PulseDE, other services) ----
  call(endpoint: string, method: string, payload?: any): Promise<any>;

  // ---- Signals ----
  emit(endpoint: string, signal: string, detail?: any): void;
  on(endpoint: string, signal: string, callback: (detail: any) => void): Unsubscribe;

  // ---- Capability scoping ----
  connect(callerId: string, capabilities: string[]): PulseIPCConnection;
}

interface PulseIPCConnection {
  call(endpoint: string, method: string, payload?: any): Promise<any>;
  on(endpoint: string, signal: string, callback: (detail: any) => void): Unsubscribe;
  emit(endpoint: string, signal: string, detail?: any): void; // only for endpoints the caller owns
}

type MethodHandler = (payload: any, callerId?: string) => any | Promise<any>;
type Unsubscribe = () => void;

class PulseIPCError extends Error {
  code: 'ENDPOINT_NOT_FOUND' | 'METHOD_NOT_FOUND' | 'CAPABILITY_DENIED' | 'HANDLER_ERROR';
  endpoint: string;
  method: string;
}
```

### 3.1 Method call semantics
- Calling an unregistered endpoint rejects with `ENDPOINT_NOT_FOUND`.
- Calling a registered endpoint with an unknown method name rejects with `METHOD_NOT_FOUND`.
- A handler throwing rejects the caller's promise with `HANDLER_ERROR`, wrapping the original error — callers never receive a raw uncaught exception across the IPC boundary, matching the real OS expectation that one process's crash doesn't take down the caller.
- All method calls are async (return Promises) even when the underlying handler is synchronous, because the real (process-boundary) version necessarily is — code written against the prototype must not assume same-tick resolution.

### 3.2 Signal semantics
- Signals are fire-and-forget; `emit` does not return a value or await listeners.
- A signal with no listeners is a no-op, not an error — unlike method calls, which always need a registered destination.
- Listener order is not guaranteed and must not be relied upon.

---

## 4. Naming convention

Endpoints use the owning subsystem's short name: `pulse.wm`, `pulse.compositor`, `pulse.session`, `pulse.fs`, `pulse.settings`, `pulse.pkg`, `pulse.theme`, `pulse.notifications`. Methods are `camelCase` verbs (`openWindow`, `getValue`, `installPackage`). Signals are `past-tense` or `noun:event` pairs (`window:opened`, `settings:changed`) to visually distinguish a signal name from a method name at call sites.

---

## 5. Capability scoping (security boundary)

Every connection to Pulse IPC is established with an explicit, named list of capabilities — endpoint+method pairs (or wildcarded endpoint namespaces) the caller is allowed to reach. This is the architectural anchor for the application sandbox described in the platform's original security goals: a third-party Pulse Store app's IPC connection is granted only `pulse.fs:readHome`, `pulse.notifications:*`, etc., never `pulse.session:*` or `pulse.pkg:install`.

The reference implementation in this prototype performs the capability check (rejecting calls outside the granted list with `CAPABILITY_DENIED`) but does not yet enforce true process isolation — that enforcement is a native-build concern (real OS sandboxing primitives), not something JS in one address space can fully guarantee. The contract is real; the isolation guarantee is partial until the native port.

---

## 6. What depends on this

Every subsystem in this document set — Service Manager, Compositor, Window Manager, Session Manager, Filesystem API, Settings API, Package API, Theme Engine, and the Application SDK — registers its public methods as Pulse IPC endpoints and is reachable only that way. There is no second, informal path between subsystems.
