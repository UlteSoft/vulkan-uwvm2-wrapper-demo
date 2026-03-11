# Security Policy

## Supported Versions

Security fixes are prioritized for the latest state of this repository.

| Version | Supported |
| ------- | --------- |
| latest / main working tree | Yes |

Older snapshots may not receive fixes.

## Reporting a Vulnerability

Please report security-sensitive issues responsibly and avoid public disclosure before a mitigation or fix is available.

Include as much detail as possible:

- a clear description of the issue
- reproduction steps
- affected platform
- compiler and build mode
- whether the issue depends on guest WebAssembly input
- whether the issue involves host-memory access, Vulkan dispatch, or preload integration

## Scope

Examples of issues that are in scope:

- out-of-bounds memory access
- incorrect handling of preload memory delivery modes
- host-memory corruption
- use-after-free or stale-handle access
- unsafe Vulkan loader dispatch behavior
- ABI confusion that can lead to memory corruption or privilege boundary issues

## Out of Scope

Examples that are generally out of scope:

- requests for general hardening advice without a concrete vulnerability
- issues caused solely by unsupported toolchains or broken local environments
- non-security build failures
- behavior that is already documented as unsupported

## Response

Best-effort goals:

- initial acknowledgment within 72 hours
- triage after reproduction details are available
- a mitigation or fix timeline based on severity and complexity
