# spirv-webgpu-transform

SPIR-V patcher/transformer that fixes up shader binaries for consumption
by WebGPU / WGSL toolchains (used by the WebGPU rendering device driver
for shader porting from SPIR-V).

- Upstream: https://github.com/davnotdev/spirv-webgpu-transform
- Version: `285f70a8a5e1710d8b0b9c3cd88ac56d8bb12a65` (see `TAG`)
- License: see upstream `LICENSE` at the pinned SHA.

Files:

- `spirv_webgpu_transform.h` — verbatim copy of
  `ffi/spirv_webgpu_transform.h` at the above SHA (byte-for-byte match).

To update: pull the desired `davnotdev/spirv-webgpu-transform` commit,
copy `ffi/spirv_webgpu_transform.h`, then bump `TAG`.
