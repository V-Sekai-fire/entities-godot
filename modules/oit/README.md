# Order-Independent Transparency (AVBOIT)

Adaptive volumetric-boundary order-independent transparency (Drobot,
[SIGGRAPH 2025](https://advances.realtimerendering.com/s2025/content/AVBOIT_SIG2025_MDROBOT-final.pdf)).
The technique voxelises transparent extinction into a view-space 3D
buffer, integrates transmittance along Z, weights each transparent
surface by the transmittance in front of it, and composites the weighted
sum in one resolve pass, so the transparent draw order stops mattering.

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
| `splat_mode`           | enum    | Raster    | Raster draws the alpha list into the froxels; Compute splats one point per surface |
| `near_plane`           | float   | 0.1       | Frustum near for slice curve fit           |
| `far_plane`            | float   | 500.0     | Frustum far for slice curve fit            |
| `linearization_factor` | float   | 1000.0    | Log-Z curve shape; below ~10 it is linear  |

## Wiring

Enable `rendering/oit/enabled` in project settings. The technique hooks into
the Mobile transparent pass directly: there is no per-camera opt-in and no
`CompositorEffect` to attach. Forward+ and the GL Compatibility renderer
have no OIT path yet and ignore the setting.

With MSAA or multiview the Mobile renderer warns once and draws
transparents sorted, with each alpha scaled by the transmittance in front
of it. Surfaces whose blend mode is not Mix (add, subtract, multiply,
premultiplied) also take that path, after the resolve.

## Target hardware

The Mobile renderer path is the load-bearing one: Meta Quest 3 (Adreno 740,
Vulkan 1.1) ships the Mobile renderer only, and this is what the module has
to run on. Concretely that constrains:

- **Splat.** Adreno's atomic-image support is limited, so voxelisation
  `atomicAdd`s packed extinction into a plain SSBO of `uint`, not the
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

## Layout

- `shaders/oit_voxelize.glsl` — compute splat, one thread per CPU-gathered
  surface sample, behind `splat_mode = Compute`. The default raster splat
  is the scene shader's `MODE_OIT_SPLAT` depth-only variant drawing the
  alpha list into an empty framebuffer at froxel resolution; both
  `atomicAdd` packed extinction into the SSBO froxel that owns the
  fragment's view position.
- `shaders/oit_integrate.glsl` — compute shader, one workgroup per (x, y)
  column, walks the Z stack with a Hillis-Steele prefix sum in shared
  memory and writes the transmittance-at-slice into an RGBA8 3D buffer.
- `shaders/oit_resolve.glsl` — fullscreen composite of the accumulation
  targets over the opaque framebuffer.
- `scene_forward_mobile_inc.glsl` (in the renderer tree) — bindings 26 to
  28 and `oit_transmittance_in_front(screen_uv, view_z)`, the lookup of
  the slice in front of the fragment's own.
- `oit_effect.{h,cpp}` — owns the pipelines, the froxel buffers, the
  accumulation targets and the resolve draw.
- `oit_math.{h}` — the slice curve, packing, lookup and resolve as plain
  C++, mirrored by `lean/` and pinned by `tests/`.
- `oit_settings.{h,cpp}` — project settings under `rendering/oit/*`.
- `register_types.{h,cpp}` — module init.

## Compositing

The paper's slides 13 and 14. Mix-blended transparents draw in any order
into three targets: `colour += c * a * T`, `alpha += a * T` and
`extinction += -log(1 - a)`, with `T` the froxel transmittance in front
of the fragment. The resolve writes
`colour * (1 - exp(-extinction)) / alpha` with alpha `1 - exp(-extinction)`
over the framebuffer. With exact `T` the weights telescope and the result
is the sorted over-blend; `lean/Oit/Resolve.lean` and the `[OIT] resolve`
doctest cases pin that, and pin the control where the lookup scales
alpha into an ordinary over-blend and attenuates every rear surface twice.

The Mobile renderer splits its merged opaque-plus-transparent pass to do
this: the accumulation framebuffer shares the depth attachment, so opaque
depth still rejects hidden transparents.
