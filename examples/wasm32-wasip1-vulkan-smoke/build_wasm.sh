#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$script_dir/../.." && pwd)

clangxx="${CLANGXX:-clang++}"
sysroot="${UWVM2_VULKAN_WASI_SYSROOT:-/Users/liyinan/Documents/MacroModel/src/wasi-libc/build-mvp/sysroot}"

"$clangxx" \
    -o "$script_dir/vulkan_smoke.wasm" \
    "$script_dir/vulkan_smoke.cc" \
    -I"$repo_dir/include" \
    -Ofast \
    -Wno-deprecated-ofast \
    -s \
    -flto \
    -fuse-ld=lld \
    -fno-rtti \
    -fno-unwind-tables \
    -fno-asynchronous-unwind-tables \
    -fno-exceptions \
    --target=wasm32-wasip1 \
    --sysroot="$sysroot" \
    -std=c++26 \
    -mno-bulk-memory \
    -mno-bulk-memory-opt \
    -mno-nontrapping-fptoint \
    -mno-sign-ext \
    -mno-mutable-globals \
    -mno-multivalue \
    -mno-reference-types \
    -mno-call-indirect-overlong
