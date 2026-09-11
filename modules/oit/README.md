# Order-Independent Transparency Module

Weighted-blended order-independent transparency (McGuire & Bavoil 2013)
implemented entirely inside a `CompositorEffect` subclass using the public
`RenderingDevice` / `CompositorEffect` API. No engine core patches.

The short slug `oit` appears only in the directory name and in the
`initialize_oit_module` / `uninitialize_oit_module` entry-point names (per
Godot module convention). Every C++ class, header basename, ClassDB
registration, doc-XML class name, and ProjectSetting path spells out
`OrderIndependentTransparency` / `order_independent_transparency`.

## Approach

1. **Accumulate pass** (2-MRT):
   * Attachment 0 — `R16G16B16A16_SFLOAT` accum, blend `ONE / ONE / ADD`.
   * Attachment 1 — `R8_UNORM` revealage, blend `ZERO / ONE_MINUS_SRC_COLOR / ADD`.
   * Weight function: `w = clamp(pow(a + 0.01, 3) * 32 / (1 + z * 0.1 * average_depth_complexity), 0.01, 32)`.
   * Reverse-Z depth reject via `texelFetch(scene_depth, ...)`.
2. **Composite blit** — fullscreen triangle, blend `ONE / ONE_MINUS_SRC_ALPHA`,
   scaled by `ldr_scale` (0.5 on LDR, 1.0 on HDR).

## Callback timing

* **Forward+**: `EFFECT_CALLBACK_TYPE_PRE_TRANSPARENT` (the built-in
  transparency pass has not yet run).
* **Mobile**: users should flip the callback to
  `EFFECT_CALLBACK_TYPE_POST_TRANSPARENT`.

## Project settings

All under `rendering/order_independent_transparency/`:

| Key                        | Type  | Default | Meaning                                            |
| -------------------------- | ----- | ------- | -------------------------------------------------- |
| `enabled`                  | bool  | false   | Master switch                                      |
| `ldr_scale`                | float | 0.5     | Composite output scale                             |
| `msaa_enabled`             | bool  | false   | Per-sample composite resolve                       |
| `rt_format`                | int   | 1 (HDR) | 0 = LDR, 1 = HDR accum precision                   |
| `average_depth_complexity` | float | 4.0     | Weight-function tuning knob                        |
| `debug_overlay`            | bool  | false   | Async readback + `order_independent_transparency_stats` signal |

## Wiring a scene

1. Add a `Compositor` resource to the camera's environment; attach an
   `OrderIndependentTransparencyCompositorEffect` to its
   `compositor_effects` list.
2. Add an `OrderIndependentTransparencyGeometryRelay` node somewhere in the
   scene (Path A). Set its `compositor_effect_path` to the effect resource
   holder, or connect `transparency_batches_ready` yourself.
3. Parent (or list in `mesh_paths`) the transparent `MeshInstance3D` nodes
   you want run through the effect.

## Extension hooks

`OrderIndependentTransparencyCompositorEffect` exposes two protected virtuals
for TressFX-style hair / PPLL back-ends to subclass without forking the
module:

* `_prepare_batches()` — runs before the MRT accumulate draw list.
* `_finalize_composite()` — runs before the composite blit.

## Path B (future engine hook, not shipped)

A future PR would add a surface-skip flag to
`RendererSceneRenderRD::_render_scene` and expose the `RENDER_LIST_ALPHA`
filter so engine-managed transparent surfaces flow through this effect
without the Path A duplication. That change is deliberately out of scope for
the first cut; the seam is documented so a follow-up can add it without
redesigning this module.

## MSAA caveat

Weighted-blended order-independent transparency with MSAA needs per-sample composite resolve (`sampler2DMS`,
`gl_SampleID`). Some drivers refuse the read-only depth attachment used
during the accumulate pass; the effect warns once and falls back to
non-MSAA.

## Known limitations

* Ghosting on low-alpha layers is inherent to weighted-blended order-independent transparency — expected
  behaviour, not a bug.
* Path A duplicates transparent vertex data on the GPU (see Path B above).
* No editor gizmo yet for the geometry relay.

## References

* Morgan McGuire and Louis Bavoil, *Weighted Blended Order-Independent
  Transparency*, JCGT 2 (2), 2013.
  <https://jcgt.org/published/0002/02/09/>
