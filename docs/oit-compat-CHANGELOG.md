# OIT on the Compatibility renderer: frontier record

Each entry is one iteration of frontier learning: what was tried, whether frontier success rose, what was kept.
A level is a configuration family (target size, froxel tile in pixels, slice count, view count). A problem is one instance:
a WebGL2 `MAX_TEXTURE_SIZE` of 2048, 4096 or 8192, and a seed for the splat set. A problem is solved when the atlas fits
inside the texture limit and a 128 MiB budget (RGBA32F) and float additive accumulation matches the `atomicAdd` reference.
Harness: `misc/scripts/oit_frontier.cpp`. A change stays only when total frontier success rises and no level loses.

## Iteration 2: pack four slices into one RGBA texel

- Baseline (one slice per texel): 460 of 540 problems solved over 36 levels.
- Candidate (four slices per texel, extinction in channel `z % 4`): 535 of 540. No level lost a problem. Kept.
- Levels that went from 0 to 15 of 15: 1080p / 128 slices / 2 views, and headset / 64 slices / 2 views, headset / 128 slices / 1 view.
- Still failing: headset / tile 6 / 128 slices / 2 views, at 10 of 15 (5 problems at `MAX_TEXTURE_SIZE` 2048). Next level to attack.
- Control: a planted fault where the integrate pass reads through a different mapping than the splat wrote scores 0 of 540 and is rejected.
- First planted fault was wrong: swapping tile and channel stays a bijection and scored 535 of 540, so the rule accepted it. It was replaced.

## Iteration 1: models of the scan and the splat

- Scan: log2(slices) ping-pong passes match a sequential prefix sum (worst error 4.6e-05, 750 columns). Planted stride and short-pass faults caught 40 of 40.
- Splat: atlas placement is a bijection for five shapes; float accumulation sits inside the 1/65536 quantisation bound (0.265 of it). Planted placement and linear-depth faults caught.
- Not yet checked: float blending and render-target formats on a real WebGL2 context.
