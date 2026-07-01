# Pulse Filesystem API

**Layer:** Pulse API
**Depends on:** Pulse IPC, Pulse Service Manager
**Depended on by:** Pulse Explorer, Pulse Application SDK, any app needing file access

---

## 1. Purpose

Pulse Filesystem API is the only path any app or PulseDE surface uses to read or manipulate files. No app code calls a real filesystem API directly (in a native build: no raw POSIX file calls from app code) — everything goes through this contract, which is also the enforcement point for the application sandbox referenced in Pulse IPC §5 (an app's IPC capability grant might scope it to `pulse.fs:readHome` only, never `pulse.fs:*`, giving each app a confined view of the filesystem rather than ambient access to the whole disk).

This is a direct, documented promotion of the original prototype's `FilesystemService`, now expressed as a real interface contract with the operations a file manager and other apps actually need (list, read, write, create, delete, move) rather than only the read-only `list()` the original prototype had.

---

## 2. Core concepts

- **Path** — an array of path segments (`['Home', 'Documents']`), not a raw string, to avoid path-separator ambiguity at the API boundary and make traversal explicit and inspectable at every call site — a small but deliberate hardening choice given this boundary is also the sandbox enforcement point (§1).
- **Entry** — a file or folder's metadata: name, type, kind (for files — document/image/archive/etc., used by Icon System §7's file-type icon selection), size, modified date.
- **Handle-free reads** — `readFile`/`writeFile` are single-shot, not stream-based, in this version of the contract; streaming is a noted future extension, not implemented, since the prototype's simulated filesystem has no real I/O to stream.

---

## 3. Interface contract

```ts
interface PulseFilesystemAPI {
  list(path: string[]): Entry[];
  stat(path: string[]): Entry | null;

  readFile(path: string[]): Promise<string>;          // text content (prototype scope — see §5)
  writeFile(path: string[], content: string): Promise<void>;

  createFolder(path: string[], name: string): Promise<Entry>;
  createFile(path: string[], name: string, content?: string): Promise<Entry>;
  delete(path: string[]): Promise<void>;
  move(fromPath: string[], toPath: string[]): Promise<void>;
  rename(path: string[], newName: string): Promise<void>;

  rootLabel(): string;
}

interface Entry {
  name: string;
  type: 'file' | 'folder';
  kind?: 'document' | 'image' | 'archive' | 'disk' | 'audio' | 'video' | 'other'; // files only
  size?: string;
  modified?: string;
}
```

### 3.1 IPC surface
Registered as `pulse.fs`. All interface methods exposed 1:1. Signals: `fs:changed` `{ path }` — emitted on any write/create/delete/move/rename, so Pulse Explorer (or any other app with an open folder view) can refresh without polling, the same way a real filesystem watcher (inotify) would drive a file manager's live view.

---

## 4. Sandbox scoping (relationship to Pulse IPC §5)

A third-party app's IPC capability grant typically scopes `pulse.fs` to a narrow root (e.g. an app-private storage folder plus explicit user-granted folders via a file picker, mirroring how Flatpak/portal-based sandboxing works on real Linux desktops) rather than the full `Home` tree. First-party apps like Pulse Explorer are granted broader scope because they *are* the file management surface. This document specifies the mechanism (capability-scoped IPC connections); specific per-app grants are a Pulse Store / installation-time concern, not something this API itself decides.

---

## 5. Honest scope of the reference implementation

The reference implementation in `fs/pulse-fs.js` is an in-memory simulated tree (the same fixture data as the original prototype's `FilesystemService`), and `readFile`/`writeFile` operate on a simple in-memory string store keyed by path — there is no real disk I/O, no binary file support, and no actual persistence across a page reload. The interface contract is written to the shape a real implementation needs (async, path-array-based, signal-driven change notification) specifically so swapping the in-memory tree for real system calls later requires no change to any caller.
