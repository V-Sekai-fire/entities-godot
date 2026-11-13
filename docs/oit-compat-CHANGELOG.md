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

## Iteration 3: froxel against a per-pixel sorted list (`misc/scripts/oit_compare.cpp`)

Answer key: every fragment sorted far to near and composited with "over". A problem is one pixel; solved when |error| <= 0.01.
4500 pixels (5 fragment counts, 3 alpha profiles, 3 depth spreads, 100 seeds). CPU model, not GPU time.

- froxel (engine design, 128 slices on the fixed 0.1-60 curve): 2694/4500, mean error 0.038.
- sorted by depth peeling, K=4 / 8 / 16 layers: 2950 / 3895 / 4269 of 4500, mean error 0.028 / 0.010 / 0.0018.
- Controls fail as required: froxel with no transmittance lookup 944/4500; peel with K=1 384/4500.
- Froxel loses most when fragments share a slice: "tight" spread (0.2 deep at depth 10) solves 318 of 1500, "wide" 1419 of 1500.
- Memory at 1680x1760 x2 eyes, model: froxel 249 MiB (atlas 90 MiB, doubled by the scan, plus 68 MiB accumulate targets), peel K=8 361 MiB, K=16 722 MiB.
- Passes over the transparent list: froxel 2, peel K.
- Not modelled: the engine's auto-range (`oit_auto_range`), which puts slices where the geometry is. The fixed curve probably understates the froxel on tight scenes. Peeling cost is an upper bound (early-z may cut shaded fragments).

## Iteration 4: the engine's auto-range, and a method that needs no sort

Same 4500 pixels and answer key as iteration 3. `oit_auto_range` (margin 0.02 m, from `render_forward_mobile.cpp:3899-3929`) sets near, far
and the log curve from the translucent geometry's depth extent. The harness uses each scene's own spread as that extent.

- froxel on the fixed 0.1-60 curve: 2694/4500. froxel with auto-range: 4226/4500, mean error 0.0035. Sorted K=16 solves 4269 with 16 passes and 722 MiB.
- The tight spread (0.2 deep at depth 10) goes from 318 to 1386 of 1500. The loss in iteration 3 was the slice placement, not the method.
- Weighted blended OIT (McGuire and Bavoil), one pass, no sort, 51 MiB: 866/4500, mean error 0.079. Its depth weight barely helps: w=1 scores 944. Rejected on accuracy.
- Control: auto-range with no transmittance lookup scores 944/4500, as the other no-lookup control does.
- The harness feeds auto-range the exact scene extent. The engine takes it from instance AABBs, which are looser. `misc/scripts/oit_compare_loose.cpp` pads the bounds:
  exact 4226, +-25% +-0.1 m 4026, +-50% +-1 m 3571, +-100% +-2 m 3270 (tight spread 1386 / 1264 / 912 / 713). The real score lies on that line.
- The engine has `oit_interval_ranges`, which splits the slices over several depth intervals when the geometry is in separate bands. It is not modelled here.
- Decision: the froxel is the sort-free method to build. Not tested on a GPU.
