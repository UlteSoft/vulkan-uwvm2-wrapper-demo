# Contributing to UWVM2 Vulkan Wrapper

This repository is a focused derivative-style companion project for the UWVM2 ecosystem.
The main priorities are:

- ABI stability
- portability
- predictable preload-plugin behavior
- clean repository organization
- compatibility with the documented UWVM2 preload memory contract

## Language and Build Expectations

- Use C17 for guest-facing C validation where appropriate.
- Use C++20 for the wrapper implementation unless the project is deliberately upgraded.
- Keep the build warning-clean.
- Keep `xmake` as the primary build entry point.

Before submitting changes, verify at least:

```bash
xmake f -m release --build-tests=y --warnings-as-errors=y
xmake -vD
./build/<plat>/<arch>/<mode>/memory_access_test
./build/<plat>/<arch>/<mode>/module_registry_test
```

## Repository Conventions

- Public headers live in `include/uwvm2_vulkan/`.
- Internal implementation headers live under `src/`.
- Source files in `src/` use `.cc` or `.cpp`-style C++ implementation naming already established by the repository.
- Test files live in `test/`.
- Keep all repository-facing filenames and documentation in English.

## API and ABI Rules

- Do not break the guest-facing ABI casually.
- Keep packed guest ABI structs trivial and byte-layout stable.
- Keep the preload exported symbol names stable unless a versioned migration path is introduced.
- Do not rely on undocumented private UWVM2 runtime layouts when the stable preload API already covers the use case.

## Memory Handling Rules

Changes that touch memory access must preserve correct handling for:

- `none`
- `copy`
- `mmap_full_protection`
- `mmap_partial_protection`
- `mmap_dynamic_bounds`

Correctness is more important than micro-optimizations.

## Vulkan Backend Rules

- Prefer runtime-loaded symbols over hard build-time coupling when possible.
- Keep platform assumptions explicit.
- Do not introduce a mandatory dependency on local Vulkan SDK headers unless the repository explicitly decides to move in that direction.

## Documentation

Update documentation when you change:

- exported guest headers
- build options
- artifact names
- supported platforms
- preload or weak-symbol integration behavior

## Pull Requests

A good pull request should:

- explain the motivation
- describe ABI or behavioral impact
- mention testing performed
- keep unrelated refactors separate when practical

## License

By contributing to this repository, you agree that your contributions are provided under the Apache License, Version 2.0.
