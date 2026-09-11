# wgpu

Public C API headers for `wgpu-native`, the Rust WebGPU implementation.

- Upstream: https://github.com/gfx-rs/wgpu-native
- Version: `12a4b75bd966a03172e5fb5a367cb2e0ce9480ed` (see `TAG`)
- License: Apache-2.0 OR MIT (upstream dual license)

Files:

- `wgpu.h` — verbatim copy of `ffi/wgpu.h` at the above SHA.
- `webgpu.h` — verbatim copy of the pinned `ffi/webgpu-headers` submodule
  from https://github.com/webgpu-native/webgpu-headers at
  `7d3186c3dd2c708703524027b46b8703534ab3cc` (the submodule pointer of
  wgpu-native at `12a4b75b`).

To update: pull the desired `wgpu-native` commit, copy `ffi/wgpu.h` and its
pinned `ffi/webgpu-headers/webgpu.h`, then bump `TAG`.
