# OIT status

Branch `dev/order-independent-transparency` (written at c0b8563f42). OIT now works from hair scale to solar-system scale on Forward+ and Mobile, with `rendering/oit/auto_range` on by default.

## What it does
- Froxel slices (up to 512) follow the translucent geometry in view, with log-depth slices.
- The translucent bounds are merged into up to 8 depth intervals and the slices are shared between them, so a far window does not coarsen hair layers.
- The lookup uses the tile centre with slope-extrapolated depth.

## Measured (`misc/oit_scenes/oit_defects.gd`, `oit_layers.gd`, OIT_SCALE 1 to 1e9, both renderers)
- Alpha-clipped crossing circles: pass (0.5/255).
- Skinned layers 2 mm apart, default bounds: pass.
- Pair 2 mm apart in front of a window 25 m behind: pass.
- Range sweep shimmer: 22/255 before, 0 now.
- `oit_cross` 0.0046, `oit_layers` 0 wrong at every scale, 33 doctests pass.

## Known limits
- Compute splat mode (`rendering/oit/splat_mode=1`) is wrong everywhere (28/255 on clipped edges). It keeps a single range.
- A loose `custom_aabb` widens its own interval and merges nearby layers (28/255 in the test).
- Layer edges can be off by about one pixel (the lookup is per 6 px tile).
- Hair stack at 1e6 and above: 68 of 15096 pixels (0.45%) over 8/255.
- GPU cost: on 60 full-screen layers at 1080p (Forward+, Apple GPU) 12.7 ms with one range vs 16.4 to 17.6 ms with three intervals. Not measured on Mobile.
- Untested: PCVR / multiview on a headset.

## Not done
- Lean spec for the intervals.
- Per-column range.
- Test in a real scene (Kimono x-ray in the Charuco Viewer).
