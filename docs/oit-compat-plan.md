# OIT on the Compatibility renderer: engine plan

Status: plan only. No engine code written. Read from `drivers/gles3` on `9d98b70bdc`.

## Where it hooks in

`RasterizerSceneGLES3::render_scene` (`rasterizer_scene_gles3.cpp:2427`):

- Alpha list is sorted at line 2612 (`sort_by_reverse_depth_and_priority`).
- The opaque colour pass ends near 2932; sky at 2948.
- Transparent pass: line 2988-2992, `_render_list_template<PASS_MODE_COLOR_TRANSPARENT>` over `render_list[RENDER_LIST_ALPHA]`, with `scene_state.enable_gl_blend(true)` set just before.
- Post processing starts at 3011 (`_render_post_processing`).

Per-surface blend state is chosen in `_render_list_template` at 3478-3520 (`BLEND_MODE_MIX/ADD/SUB/MUL/PREMULT_ALPHA/ALPHA_TO_COVERAGE`).

Shader variants come from `#[modes]` in `drivers/gles3/shaders/scene.glsl` (`mode_color`, `mode_depth`, plus instancing forms). `USE_MULTIVIEW` is a specialization constant.

## Mapping from the RD design

| RD (`servers/rendering/renderer_rd`) | Compatibility (this plan) |
| --- | --- |
| `_oit_prepass` voxelize: compute `atomicAdd` into a storage buffer | New scene mode `mode_oit_splat`: draw each alpha-list surface's fragments, write extinction `-log(1-alpha)` additively into an RGBA32F atlas texture (blend `ONE, ONE`; verified on Chromium, Firefox, Steam Frame Firefox) |
| `oit_integrate.glsl`: shared-memory prefix sum | ping-pong full-screen passes over the atlas (`oit_scan_math.h`) |
| `MODE_OIT_ACCUMULATE` in the Mobile fragment | New scene mode `mode_oit_accumulate`: looks up transmittance in front, writes 3 targets (colour, alpha, extinction) with additive blend, MRT |
| `oit_resolve.glsl` | full-screen resolve blended over the colour buffer |

New files: `drivers/gles3/effects/oit.{h,cpp}` (atlas, scan passes, resolve), `drivers/gles3/shaders/effects/oit_scan.glsl`, `oit_resolve.glsl`. Edits: `scene.glsl` (two modes), `rasterizer_scene_gles3.cpp` (prepass before line 2988; accumulate + resolve in place of the transparent call when `rendering/oit/enabled`), project settings already exist.

## Open problems, in the order they bite

1. **The packed scan is not modelled.** `oit_scan_math.h` scans one value per slice in 7 passes. The kept layout packs four slices into one texel (channel `z % 4`). The scan over that layout is an in-texel prefix, then a Hillis-Steele pass over tiles (32 tiles for 128 slices, 5 passes), then adding each tile's carry. That needs its own model and planted faults before any GL code.
2. **Splat draws from fragments, not points.** The RD Compute mode splats one point per surface sample. In GLES3 the surface is rasterised, so a fragment can only write its own pixel. It cannot choose its slice by writing elsewhere. Options: one draw per slice-group with a discard on `slice != group` (four groups, using four MRT channels), or point sprites from a vertex-pulled sample list. Not decided. The RD Raster mode (`splat_mode=0`) was not read.
3. **Lookup filtering.** RD uses `texture()` on a 3D texture (trilinear). The atlas needs two taps and a manual lerp in z, plus the half-texel clamp in x from `oit_transmittance_in_front`.
4. **Multiview.** `OVR_multiview2` is false on Metal Chromium, Firefox and the Steam Frame Firefox. XR needs per-eye draws into the shared packed atlas (`oit_packed_column`).
5. **MSAA and the 3 accumulate targets** share the existing `fbo_msaa_3d` path; resolve under MSAA is not designed.

## Gates

Each step above ships with a standalone check that has a planted fault, as `misc/scripts/oit_scan_check.cpp` and `oit_splat_check.cpp` do, and a row in `docs/oit-compat-CHANGELOG.md`. The first GL step is not done until `checks/run.sh`-style parity against the RD output on the stack scene passes within the existing band.
