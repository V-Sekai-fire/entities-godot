# Web link fixes for the assembled tree (kept, not applied)

`emscripten-6.0.12-link.patch` was made against entities-zone tag `v0.0.0-20261112175806-8c97c7ac1df7`
(`8c97c7ac1d`). It let the web template link with Emscripten 6.0.12 (the version CI pins):

- `platform/web/detect.py`: drop `-sDISABLE_EXCEPTION_CATCHING=0` and `-sDISABLE_EXCEPTION_THROWING=0`
  (they conflict with `-fwasm-exceptions` and `-sSUPPORT_LONGJMP=wasm`).
- `drivers/png/SCsub`, `modules/jpg/SCsub`, `modules/freetype/SCsub`: compile with
  `-sSUPPORT_LONGJMP='wasm'` instead of `'emscripten'` so they match the link flag.

The build also needed `module_sqlite_enabled=no module_pixal3d_enabled=no module_open_telemetry_enabled=no`.
Emscripten 7.0.0 rejects the flags regardless of module set. No browser run of OIT completed: with XR shaders on,
Chromium's WGSL compiler rejects `enable view_instancing`; with them off the page died with an uncaught `Exception`.
Not pushed to any engine branch. WebGPU is blocklisted (2026-11-12), so this is a record.
