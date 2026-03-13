# UWVM2 Vulkan Wrapper

`uwvm2_vulkan_wrapper` is a standalone Vulkan host-wrapper project for **UWVM2**.
It provides:

- a guest-facing C header surface under [`include/uwvm2_vulkan/`](include/uwvm2_vulkan)
- a preload dynamic-library implementation for UWVM2
- a weak-symbol registration form for ELF-style integration
- a stable memory-access layer that handles every currently documented UWVM2 preload memory delivery mode
- a plugin-facing WASI Preview 1 capability bridge that can reuse the guest's UWVM2 rights-scoped environment for file-system operations

The project is organized as a derivative-style companion repository rather than a monolithic part of the main `uwvm2` tree.

## Status

This repository is currently **developer-oriented**.

The codebase already includes:

- a preload-DL export path
- a weak-symbol export path
- dynamic Vulkan loader dispatch
- physical-device feature and extension queries
- device and queue idle synchronization entry points
- command pool creation, reset, and destruction
- command buffer allocation, free, begin, end, and reset entry points
- shader module creation and destruction
- semaphore and fence synchronization primitives
- buffer and image resource lifecycle wrappers
- buffer/image memory requirement and memory binding wrappers
- plugin-side reuse of the exported UWVM2 WASI Preview 1 host API for capability-scoped file access
- guest-memory adaptation for:
  - `none`
  - `copy`
  - `mmap_full_protection`
  - `mmap_partial_protection`
  - `mmap_dynamic_bounds`
- local validation targets for memory and module-registry behavior

The current implementation focuses on a stable plugin architecture and a clean guest ABI surface.

## Repository Layout

```text
include/uwvm2_vulkan/
  attributes.h
  wasi_snapshot_preview1.h
  vulkan.h
  vulkan_types.h

src/
  abi/
  plugin/
  runtime/
  uwvm2_vulkan/
  vulkan/

test/
  guest_header_smoke.c
  memory_access_test.cc
  module_registry_test.cc
  wasi_file_system_test.cc
  vulkan_api_test.cc

xmake/
  impl.lua
  option.lua
```

## Key Design Goals

- Keep the guest-facing interface minimal, stable, and easy to consume from C and C-compatible WebAssembly toolchains.
- Follow the UWVM2 preload ABI model instead of depending on private runtime layouts.
- Preserve correctness across all supported memory delivery modes.
- Keep the Vulkan backend dynamically loaded so the project can build without a hard dependency on Vulkan SDK headers in the wrapper itself.
- Maintain a layout and build style that feels natural for a UWVM2 derivative project.

## Build Requirements

- `xmake` 2.9.8 or newer
- a C17 compiler
- a C++20 compiler
- a platform toolchain supported by the top-level `xmake.lua`

The wrapper does not require Vulkan headers at build time because it uses an internal native ABI shim plus runtime symbol loading.

## Build

Configure and build in release mode:

```bash
xmake f -m release
xmake
```

Build with tests enabled and warnings treated as errors:

```bash
xmake f -m release --build-tests=y --warnings-as-errors=y
xmake -vD
```

## Build Options

The repository currently exposes these primary Xmake options:

- `build-tests`
- `merge-weak-object`
- `stdlib`
- `fno-exceptions`
- `warnings-as-errors`

Inspect them with:

```bash
xmake f --help
```

## Output Artifacts

Typical outputs include:

- `libuwvm2_vulkan_plugin_core.a`
- `libuwvm2_vulkan_plugin_dl.so` / `.dylib` / `.dll`
- object files for the weak-symbol form
- local test binaries when `build-tests` is enabled

On ELF-oriented platforms, `merge-weak-object` also produces a merged relocatable object:

- `uwvm2_vulkan_plugin_weak.o`

## Guest Headers

The guest headers are intended for WebAssembly-side code:

- [`include/uwvm2_vulkan/wasi_snapshot_preview1.h`](include/uwvm2_vulkan/wasi_snapshot_preview1.h)
  Provides a small set of WASI Preview 1 import declarations.
- [`include/uwvm2_vulkan/vulkan_types.h`](include/uwvm2_vulkan/vulkan_types.h)
  Defines the guest-visible Vulkan wrapper ABI structs and constants.
- [`include/uwvm2_vulkan/vulkan.h`](include/uwvm2_vulkan/vulkan.h)
  Declares the imported wrapper entry points exposed through the UWVM2 Vulkan module.

## Integration Model

### Preload dynamic library

The preload-DL form exports:

- `uwvm_set_preload_host_api_v1`
- `uwvm_set_wasip1_host_api_v1`
- `uwvm_get_module_name`
- `uwvm_get_custom_handler`
- `uwvm_function`

This is the preferred route when the wrapper needs the stable preload host API for memory access and the plugin-facing WASI Preview 1 host API for rights-scoped filesystem reuse.

### Weak symbol registration

The weak-symbol form exports:

- `uwvm_weak_symbol_module`

The weak-symbol module descriptor also carries both host-API setter callbacks so the runtime can inject preload-memory access and WASI capability state in the same style as native UWVM2 derivative modules.

## WASI Capability Reuse

The wrapper does not treat the host process as its filesystem authority.
When filesystem-oriented helper code is needed inside the plugin, it is expected to go through the plugin-facing `uwvm_wasip1_host_api_v1` surface exposed by UWVM2.

That means:

- path and descriptor operations can be constrained by the same WASI rights/capability model granted to the guest module
- plugins can reuse preopened directories and rights already configured by the embedding UWVM2 runtime
- wrapper-side file access can remain aligned with guest-visible capability boundaries instead of silently inheriting unrestricted host-process permissions

## Memory Delivery Compatibility

The runtime layer adapts all currently targeted UWVM2 preload memory delivery states:

- `none`
  Access is rejected.
- `copy`
  Reads and writes go through the host API callbacks.
- `mmap_full_protection`
  Direct pointer access is used after bounds validation.
- `mmap_partial_protection`
  Direct pointer access is used with both partial-region and full-length checks.
- `mmap_dynamic_bounds`
  Effective bounds are refreshed from the dynamic-length atomic object before direct access.

## Local Validation

Current local validation targets:

- `memory_access_test`
- `module_registry_test`
- `wasi_file_system_test`
- `vulkan_api_test`
- `guest_header_smoke`

Run the tests after building:

```bash
./build/<plat>/<arch>/<mode>/memory_access_test
./build/<plat>/<arch>/<mode>/module_registry_test
./build/<plat>/<arch>/<mode>/wasi_file_system_test
./build/<plat>/<arch>/<mode>/vulkan_api_test
```

## License

This project is licensed under the Apache License, Version 2.0.
See [LICENSE.md](LICENSE.md).

## Security

Please report security-sensitive issues responsibly.
See [SECURITY.md](SECURITY.md).

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) before submitting changes.
