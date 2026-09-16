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
7. Composite the normalised transparency against the opaque background.

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

## Wiring a scene

1. Attach an `OITCompositorEffect` to the camera environment's `Compositor`
   resource, in its `compositor_effects` list.
2. Enable `rendering/oit/enabled` in the project settings.

## State

This is the scaffold on top of which the paper's algorithm is being built.
The voxelise / integrate compute pipelines and the Forward+ transparent
shader patch that samples the integrated extinction land in follow-up
commits; the C++ effect and its project settings register cleanly against
`master` first so the surface area is reviewable in isolation.
