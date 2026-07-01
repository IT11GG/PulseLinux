# Security Policy

## Supported Versions

PulseLinux is currently in pre-release development. There are no stable release
versions yet. Security issues found in the development tree should still be
reported through the process below.

## Reporting a Vulnerability

**Do not report security vulnerabilities through public GitHub issues.**

Email: security@pulselinux.org

Include:
- Description of the vulnerability
- Steps to reproduce
- Potential impact assessment
- Any suggested fix (optional)

You will receive an acknowledgement within 72 hours and a full response within
7 days. We ask for responsible disclosure — please allow 90 days for a fix to
be developed and released before publishing details publicly.

## Scope

- The PulseDE Wayland compositor (`src/pulse-de/`)
- The Pulse Runtime IPC layer (`src/pulse-runtime/`)
- The PulsePkg package manager (`src/pulsepkg/`)
- Any privilege escalation paths in system services

Vulnerabilities in upstream dependencies (wlroots, libwayland, Linux kernel)
should be reported to their respective projects.
