# Pulse Package API

**Layer:** Pulse API
**Depends on:** Pulse IPC, Pulse Service Manager
**Depended on by:** Pulse Store (app), Pulse Settings → Updates panel, Pulse Application SDK

---

## 1. Purpose

Pulse Package API is the programmatic surface over PulsePkg — the package manager named in the project's original brief (binary repositories, delta updates, rollback, signed packages). This document defines the interface every consumer (Pulse Store's install button, Settings → Updates, a future `pulsepkg` CLI) calls, rather than each consumer reimplementing dependency resolution or download/verify logic itself. The CLI and the GUI are two callers of one API, not two independent implementations, which is the direct architectural reading of the original brief's "Easy CLI / Beautiful graphical integration" as two surfaces over one engine.

---

## 2. Core concepts

- **Package** — id, name, version, publisher, signature status, installed/available state, size, and dependency list.
- **Operation** — an install/update/remove/rollback in progress, with a stable id so callers can track progress via signals rather than polling, and so the Notification Center (HIG §6) can show a determinate progress bar (Component Library §4) backed by real operation progress rather than a fake animation.
- **Rollback point** — a snapshot reference created automatically before any install/update, allowing `rollback(packageId)` to restore the previous version — direct support for the brief's "Rollback" requirement at the API level, not just a UI affordance.
- **Repository** — a configured package source with a priority (brief: "Repository priorities"); query/install operations resolve across all enabled repositories in priority order.

---

## 3. Interface contract

```ts
interface PulsePackageAPI {
  search(query: string): Promise<PackageInfo[]>;
  getInfo(packageId: string): Promise<PackageInfo | null>;
  listInstalled(): Promise<PackageInfo[]>;
  listUpdatable(): Promise<PackageInfo[]>;

  install(packageId: string): OperationHandle;
  remove(packageId: string): OperationHandle;
  update(packageId: string): OperationHandle;
  updateAll(): OperationHandle;
  rollback(packageId: string): OperationHandle;

  listRepositories(): RepositoryInfo[];
  setRepositoryPriority(repoId: string, priority: number): void;
}

interface PackageInfo {
  id: string; name: string; version: string; publisher: string;
  signed: boolean;
  installed: boolean;
  size: string;
  description?: string;
  dependencies: string[];
  category: 'system' | 'productivity' | 'gaming' | 'development' | 'media' | 'other';
}

interface OperationHandle {
  operationId: string;
  promise: Promise<void>; // resolves on completion, rejects on failure
}

interface RepositoryInfo { id: string; name: string; url: string; priority: number; enabled: boolean; }
```

### 3.1 IPC surface
Registered as `pulse.pkg`. All interface methods exposed 1:1 (note `install`/`remove`/`update`/`updateAll`/`rollback` return immediately with an `operationId`; the IPC method itself resolves once the operation is *queued*, not complete — completion is tracked via signals below, matching how a real package manager's long-running download/install shouldn't block the calling IPC round-trip).

Signals: `pkg:operation-progress` `{ operationId, packageId, percent }`, `pkg:operation-complete` `{ operationId, packageId, success, error? }`, `pkg:installed` `{ packageId }`, `pkg:removed` `{ packageId }`.

---

## 4. Relationship to Pulse Store and Settings → Updates

Pulse Store's install button calls `install(packageId)` and renders the returned `operationId`'s progress (via `pkg:operation-progress`) using the Component Library's determinate progress bar inside the button's loading state (Component Library §2's "Loading state" behavior) — never a fake animation, since real percent data is available from this API. Settings → Updates' "Automatic updates" toggle (HIG §5, the settings row shown in the prototype's Settings app) is backed by `listUpdatable()` and `updateAll()` from this same API — there is exactly one update mechanism in PulseLinux, surfaced through two UI entry points.

---

## 5. Rollback as a first-class operation

Per the original project brief's explicit requirement ("Rollback" is listed under both Package Manager and Pulse Store), `rollback(packageId)` is a top-level method, not an afterthought bolted onto `install`. Every successful `install`/`update` operation implicitly creates a rollback point before applying changes (reference implementation: keeps the previous `PackageInfo` version in memory); `rollback` restores it and emits the same `pkg:operation-complete` signal shape as any other operation, so UI code doesn't need a special case to display rollback progress differently from a normal update.

---

## 6. Honest scope of the reference implementation

The reference implementation in `pkg/pulse-pkg.js` simulates a small fixed catalog of packages with fake network/install delays (`setTimeout`-paced progress signals) rather than real downloads, dependency resolution, or cryptographic signature verification. The interface contract — operation handles with progress signals, rollback as a real operation, repository priority — is written to the shape a real PulsePkg engine needs; the simulated timing and fixed catalog are prototype-only stand-ins, matching this document set's consistent practice of keeping the contract honest even where the implementation is a simulation.
