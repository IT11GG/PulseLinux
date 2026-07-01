# Pulse Service Manager

**Layer:** System Services
**Depends on:** Pulse IPC
**Depended on by:** Pulse Session Manager, and indirectly everything that needs a service started before it can be called

---

## 1. Purpose

Pulse Service Manager is PulseLinux's systemd-equivalent for user-session services: it knows what services exist, what each depends on, starts them in correct dependency order, restarts ones that crash, and exposes their live status. Every other platform subsystem in this document set (Compositor, Window Manager, Filesystem API, Settings API, Package API, Theme Engine) is itself registered with the Service Manager as a service — there is no special-cased bootstrap path that skips it, including for first-party "core" services.

This is the layer that answers, architecturally: *what is actually running right now, and in what order did it have to start?* — a question a real OS needs a real answer to, and a question the original prototype's `shell.js` answered only implicitly (whatever order `import` statements happened to run in).

---

## 2. Core concepts

- **Service descriptor** — a registered definition: id, human name, the module/entry point that starts it, its declared dependencies (other service ids), and whether it's a long-running daemon or a one-shot.
- **Service state** — `stopped | starting | running | failed | stopping`, tracked per service, broadcast as IPC signals on every transition.
- **Dependency graph** — Service Manager topologically sorts declared dependencies and starts services in the resulting order, refusing to start a service whose dependencies haven't reached `running`.
- **Restart policy** — `none | on-failure | always`, declared per service; a crashed `on-failure` service is restarted with backoff, with a signal emitted on every restart attempt so the Notification Center (or, in a native build, a system log) can surface persistent failures rather than silently retrying forever.

---

## 3. Interface contract

```ts
interface PulseServiceManager {
  register(descriptor: ServiceDescriptor): void;
  start(serviceId: string): Promise<void>;
  stop(serviceId: string): Promise<void>;
  restart(serviceId: string): Promise<void>;
  status(serviceId: string): ServiceStatus;
  list(): ServiceStatus[];
  startAll(): Promise<void>; // resolves once every registered service has reached 'running' or 'failed'
}

interface ServiceDescriptor {
  id: string;                 // e.g. 'pulse.compositor'
  name: string;                // human-readable, e.g. 'Pulse Compositor'
  dependsOn?: string[];        // other service ids
  restartPolicy?: 'none' | 'on-failure' | 'always';
  start: () => Promise<void> | void; // the service's actual bring-up logic
  stop?: () => Promise<void> | void;
}

interface ServiceStatus {
  id: string;
  name: string;
  state: 'stopped' | 'starting' | 'running' | 'failed' | 'stopping';
  startedAt?: Date;
  lastError?: string;
  restartCount: number;
}
```

### 3.1 IPC surface
Registered as `pulse.services` over Pulse IPC:

| Method | Payload | Returns |
|---|---|---|
| `start` | `{ serviceId }` | `void` |
| `stop` | `{ serviceId }` | `void` |
| `restart` | `{ serviceId }` | `void` |
| `status` | `{ serviceId }` | `ServiceStatus` |
| `list` | — | `ServiceStatus[]` |

Signals: `service:state-changed` `{ serviceId, prevState, nextState }`, `service:failed` `{ serviceId, error }`.

---

## 4. Dependency resolution

On `startAll()`, the Service Manager performs a topological sort over all registered descriptors' `dependsOn` lists. A cycle is a fatal configuration error (thrown at registration-resolution time, not silently ignored) — this is deliberate: a dependency cycle between system services is exactly the class of bug that should be impossible to ship silently, the same way it would be a build-breaking error in a real init system's unit files.

Services with no unmet dependencies start in parallel where possible (not strictly sequential), matching how systemd parallelizes independent units — this matters for boot-time performance goals stated in the project's original brief ("fast boot... lazy loading... background service optimization").

---

## 5. Relationship to Pulse Session Manager

Service Manager answers "is this service running"; Session Manager (next document) answers "what should the user be looking at right now" — boot, lock, login, desktop. Session Manager is itself a *consumer* of Service Manager: it calls `start()` on the services a given session phase needs (e.g., the Compositor and Window Manager must be `running` before Session Manager transitions to the desktop phase) rather than owning service bring-up logic itself. This separation is what lets either be tested or replaced independently — a goal directly inherited from the project's "clean modular architecture" requirement.

---

## 6. What this replaces from the original prototype

In `runtime/pulse-api.js`, every subsystem (`fs`, `settings`, `notifications`, `wm`) was simply instantiated inline as part of one frozen object, in source-order, with no concept of "started" vs "not yet started" and no failure handling. Pulse Service Manager makes that bring-up sequence explicit, observable, and individually restartable — which is necessary groundwork before PulseDE can honestly claim things like "fast boot" or "service health monitoring" from the original project brief.
