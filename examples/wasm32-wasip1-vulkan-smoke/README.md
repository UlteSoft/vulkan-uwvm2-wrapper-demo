# `wasm32-wasip1-vulkan-smoke`

This example is a minimal C++ WebAssembly guest for the `wasiu-vulkan` module.
It is intentionally small and only checks the currently implemented wrapper path:

- `loader_available`
- `enumerate_instance_version`
- `create_instance`
- `destroy_instance`

The source is written as a `.cc` file and compiled directly to `wasm32-wasip1`
with the same `clang++` flag style you requested for your local toolchain.

On macOS, the example also accounts for the MoltenVK portability path:

- the wrapper probes common Homebrew Vulkan and MoltenVK library locations
- the guest enables `VK_KHR_portability_enumeration` automatically when needed
- the guest requests a conservative Vulkan 1.0 instance API version

## Files

- `vulkan_smoke.cc` - the guest program
- `build_wasm.sh` - direct `clang++` build script

## Build

From the repository root:

```sh
sh ./examples/wasm32-wasip1-vulkan-smoke/build_wasm.sh
```

The script emits:

```text
examples/wasm32-wasip1-vulkan-smoke/vulkan_smoke.wasm
```

By default it uses:

- `clang++` from `PATH`
- sysroot `/Users/liyinan/Documents/MacroModel/src/wasi-libc/build-mvp/sysroot`

You can override them:

```sh
CLANGXX=/path/to/clang++ \
UWVM2_VULKAN_WASI_SYSROOT=/path/to/sysroot \
sh ./examples/wasm32-wasip1-vulkan-smoke/build_wasm.sh
```

## Run

First build the plugin:

```sh
xmake f -m release --build-tests=y --warnings-as-errors=y
xmake
```

Then run the wasm guest with UWVM2:

```sh
/Users/liyinan/Documents/MacroModel/src/uwvm2/build/macosx/arm64/release/uwvm \
  --wasm-register-dl ./build/macosx/arm64/release/libuwvm2_vulkan_plugin_dl.dylib wasiu-vulkan \
  --wasm-set-preload-module-attribute wasiu-vulkan copy all \
  --run ./examples/wasm32-wasip1-vulkan-smoke/vulkan_smoke.wasm
```

## Expected Outcomes

If a host Vulkan loader is available, the example should print:

- `loader_available=1`
- a successful `enumerate_instance_version`
- a successful `create_instance`
- a successful `destroy_instance`

If the machine does not provide `libvulkan*.dylib` or `libMoltenVK.dylib`,
the example still proves that the wasm guest and plugin wiring work, but it
will report `loader_available=0` and exit with code `2`.

## Verified Local Run

This repository was verified locally on macOS with Homebrew `molten-vk`,
Homebrew `vulkan-loader`, `uwvm2`, and the preload-DL plugin form.

Observed output:

```text
uwvm2-vulkan-smoke: loader_available=1
uwvm2-vulkan-smoke: enumerate_instance_version -> 0
uwvm2-vulkan-smoke: instance api version=1.4.341 variant=0
uwvm2-vulkan-smoke: enumerate_instance_extension_properties.count -> 5
uwvm2-vulkan-smoke: enumerate_instance_extension_properties.list -> 0
uwvm2-vulkan-smoke: portability_enumeration=enabled
uwvm2-vulkan-smoke: create_instance -> 0
uwvm2-vulkan-smoke: instance_handle=0x1
uwvm2-vulkan-smoke: destroy_instance -> 0
```
