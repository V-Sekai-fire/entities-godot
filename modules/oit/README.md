# Order-Independent Transparency (AVBOIT)

Adaptive volumetric-boundary order-independent transparency (Drobot,
[SIGGRAPH 2025](https://advances.realtimerendering.com/s2025/content/AVBOIT_SIG2025_MDROBOT-final.pdf)).
The technique voxelises transparent extinction into a view-space 3D
buffer, integrates transmittance along Z, and modulates each transparent
surface's alpha by the accumulated transmittance at its position — so
the transparent draw order stops mattering.

## Basic steps

Following slide 47 of the paper:

1. Fit a log-Z slice curve over the view frustum.
2. Clear the volumetric extinction buffer.
3. Splat transparency into the extinction buffer at reduced tile resolution.
4. Sum extinction along view rays into a 3D transmittance-integral texture.
5. Draw opaque geometry.
6. Render transparency in any order — each surface looks up the integrated
   extinction at its 3D position and multiplies its own alpha by the
   transmittance to that point.
7. Composite the normalized transparency against the opaque background.

## Project settings

All under `rendering/oit/`:

| Key                    | Type    | Default   | Meaning                                    |
| ---------------------- | ------- | --------- | ------------------------------------------ |
| `enabled`              | bool    | false     | Master switch                              |
| `slice_count`          | int     | 128       | Z slices in the froxel buffer              |
| `tile_size`            | Vector2 | (6, 6)    | Screen-space tile size for splat resolution |
| `near_plane`           | float   | 0.1       | Frustum near for slice curve fit           |
| `far_plane`            | float   | 500.0     | Frustum far for slice curve fit            |
| `linearization_factor` | float   | 0.5       | Log-Z curve shape                          |

## Wiring

Enable `rendering/oit/enabled` in project settings. The technique hooks into
the Mobile and Forward+ transparent passes directly — there is no per-camera
opt-in and no `CompositorEffect` to attach. The GL Compatibility renderer
has no OIT path and ignores the setting.

## Target hardware

The Mobile renderer path is the load-bearing one: Meta Quest 3 (Adreno 740,
Vulkan 1.1) ships the Mobile renderer only, and this is what the module has
to run on. Concretely that constrains:

- **Splat.** Adreno's atomic-image support is limited, so voxelisation uses
  R32_UINT storage-image `InterlockedAdd` with packed extinction, not the
  packed 8-bit-atomics variant from slide 53 of the paper.
- **Integrate.** Compute prefix-sum along Z stays inside a single workgroup
  per (x, y) column so the pass has no cross-workgroup barrier.
- **Slice count.** Defaults tuned to 128, one U32 per voxel — the 320x180x128
  budget from the paper is ~28 MiB, which fits Quest 3's tile memory
  headroom.
- **Subpasses.** The Mobile renderer expresses its passes as Vulkan
  subpasses; voxelise and integrate run *before* the main render pass to
  keep the transparent-fragment sample-of-the-integrated-buffer legal
  inside the subpass that draws the transparents.

Forward+ runs the same shader path with the wider budget of a desktop card.

## State

This is the scaffold on top of which the paper's algorithm is being built.
The voxelise / integrate compute pipelines and the Mobile / Forward+
transparent shader patch that samples the integrated extinction land in
follow-up commits; the project settings register cleanly against `master`
first so the surface area is reviewable in isolation.
