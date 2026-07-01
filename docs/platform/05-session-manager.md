# Pulse Session Manager

**Layer:** Pulse Runtime
**Depends on:** Pulse IPC, Pulse Service Manager
**Depended on by:** PulseDE shell (the only direct consumer — apps never touch this)

---

## 1. Purpose

Pulse Session Manager owns the single source of truth for what phase of the user session PulseLinux is in — booting, locked, at login, or at the desktop — and is the only subsystem permitted to drive transitions between those phases. PulseDE's shell renders whatever phase Session Manager reports; it does not independently decide "now show the lock screen," it reacts to a `session:phase-changed` signal exactly the way an app reacts to any other Pulse IPC signal.

This formalizes what `PowerService` did informally in the original prototype, with one structural addition required for a real platform: **phase transitions require their phase's dependent services to actually be running**, not just a state variable flip. Session Manager cannot honestly claim to have reached `desktop` until Window Manager and Compositor (via Pulse Service Manager) report `running` — in the original prototype, `loginSuccess()` flipped a state enum unconditionally regardless of whether the desktop's dependencies were ready, which is not a pattern a real OS could get away with.

---

## 2. Core concepts

- **Session phase** — `off | booting | locked | login | desktop | locking | shutting-down`. A strict state machine; only specific transitions are legal (§4).
- **Session user** — the account associated with the current (or pending) session; populated once login succeeds.
- **Phase service requirements** — each phase declares which Pulse services must be `running` before the transition into it completes (e.g., `desktop` requires `pulse.compositor` and `pulse.wm`).

---

## 3. Interface contract

```ts
interface PulseSessionManager {
  powerOn(): Promise<void>;        // off -> booting; starts core services; resolves once boot's required services are running
  bootComplete(): void;             // booting -> locked (caller signals boot work is done; phase only advances if required services are confirmed running)
  unlock(): void;                   // locked -> login
  login(credentials: Credentials): Promise<LoginResult>; // login -> desktop on success; starts desktop-phase services first
  lock(): void;                     // desktop -> locked
  powerOff(): Promise<void>;        // any phase -> shutting-down -> off; stops services in reverse dependency order

  phase(): SessionPhase;
  currentUser(): SessionUser | null;
}

type SessionPhase = 'off' | 'booting' | 'locked' | 'login' | 'desktop' | 'locking' | 'shutting-down';

interface Credentials { password: string; } // prototype: cosmetic, no real auth backend (see §5)
interface LoginResult { success: boolean; reason?: string; }
interface SessionUser { name: string; avatarInitial: string; }
```

### 3.1 IPC surface
Registered as `pulse.session`. All interface methods exposed 1:1. Signals: `session:phase-changed` `{ prevPhase, nextPhase }`, `session:login-failed` `{ reason }`.

---

## 4. Legal phase transitions

```
off ──powerOn()──▶ booting ──bootComplete()──▶ locked
locked ──unlock()──▶ login
login ──login() success──▶ desktop
login ──login() failure──▶ login   (stays, emits session:login-failed)
desktop ──lock()──▶ locked
any phase ──powerOff()──▶ shutting-down ──▶ off
```

Any call attempting a transition not listed above (e.g. calling `login()` while in `desktop`) throws rather than silently no-op-ing — an illegal phase transition is a caller bug, and Session Manager surfacing it loudly is preferable to PulseDE's shell silently doing nothing and leaving the person looking at a confusing screen.

---

## 5. Relationship to Pulse Service Manager

Each phase transition that enters `booting` or `desktop` calls into Pulse Service Manager to ensure that phase's required services are `running` before the phase change is considered complete:

| Entering phase | Required services |
|---|---|
| `booting` | `pulse.compositor`, `pulse.theme` (boot screen needs to render themed content) |
| `desktop` | `pulse.compositor`, `pulse.wm`, `pulse.fs`, `pulse.settings`, `pulse.notifications` |

If a required service fails to start, Session Manager does not advance the phase — it remains in the prior phase and surfaces the failure (in a native build, this is the architectural hook for a real "recovery mode" per the project's original system-applications brief; the prototype logs the failure rather than implementing recovery UI).

---

## 6. Authentication — honest scope

The prototype's `login()` accepts any non-empty password and always succeeds, exactly matching the original prototype's login screen. This document's contract (`Credentials`, `LoginResult`, the `session:login-failed` signal) is written as if a real PAM-equivalent auth backend sits behind it, specifically so that swapping in real authentication later is a matter of changing what `login()`'s internals call, not changing Session Manager's public contract or anything above it.

---

## 7. What this replaces from the original prototype

`runtime/pulse-api.js`'s `PowerService` was five methods flipping one private field with no legality checking and no dependency awareness. This document's Session Manager adds: enforced legal-transition checking (§4), service-readiness gating before a phase is considered "reached" (§5), and a real login result type rather than an unconditional state flip — all required before PulseDE can be implemented "using these APIs instead of accessing implementation details directly," per this round's stated goal.
