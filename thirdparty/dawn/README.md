# dawn

WebGPU implementation used by the WebGPU rendering device driver.

- Upstream: https://github.com/google/dawn
- Version: `1e897275172a23f27b0022fa6beae3084ed54a9b` (see `TAG`)
- License: BSD-3-Clause

Pruned source snapshot produced by `update-dawn.sh` from the pinned
commit shallow-cloned from upstream. Retained trees:

- `src/` — `dawn_native`, `dawn_wire`, `dawn_common`, `tint`.
- `include/` — public C and C++ headers.
- `generator/` — Python generator that emits `webgpu.h`,
  `dawn_proc_table.h`, `webgpu_cpp*.h` from `dawn.json`.
- `scripts/`.
- `third_party/` — only the Khronos header sets and `emdawnwebgpu` glue
  Dawn's sources #include.
- `CMakeLists.txt`, `CMakeSettings.json` — reference only. Dawn is
  built by an SCsub under `drivers/webgpu/`, not upstream CMake or GN.

Everything else — tests, samples, benchmarks, docs, WebGPU CTS,
Chromium build infrastructure, prebuilt SDK binaries — is dropped by
the update script.

## To update

Edit `GIT_COMMIT` in `update-dawn.sh`, run it, bump `TAG` to match.
