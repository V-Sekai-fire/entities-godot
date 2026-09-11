# naga-native

C API bindings for `naga`, the WGSL/SPIR-V/MSL/HLSL/GLSL shader translator
used by `wgpu` — packaged as a standalone C library by davnotdev.

- Upstream: https://github.com/davnotdev/naga-native
- Version: `87cd2a99b6b894a657a8bb798745699f45748094` (see `TAG`)
- License: see upstream (`LICENSE` / `Cargo.toml` at the pinned SHA)

Files:

- `naga.h` — verbatim copy of `naga.h` at the above SHA (byte-for-byte match).

To update: pull the desired `davnotdev/naga-native` commit, copy `naga.h`,
then bump `TAG`.
