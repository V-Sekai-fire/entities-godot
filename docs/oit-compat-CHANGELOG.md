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

## Iteration 5: the engine's depth intervals (`misc/scripts/oit_compare_intervals.cpp`)

Scenes of 1 to 9 translucent instances, each a depth band with an AABB looser than its fragments. 9000 pixels, 128 slices both ways.
Single = one `oit_auto_range` over the union; intervals = `OITIntervalSet` and `oit_interval_ranges` as `render_forward_mobile.cpp:3911-3935` builds them.

- Total solved: single 5938, intervals 6706 of 9000. No-lookup controls score 1544 for both, and with one interval the two paths are identical on every pixel.
- Intervals win when the AABBs are close to the fragments: 3 bands exact 390 -> 560, 5 bands exact 366 -> 529, 9 bands exact 336 -> 467.
- They lose when padding is large: at +-100% +-2 m, 3 bands 332 -> 316, 5 bands 295 -> 255, 9 bands 231 -> 163.
- Cause, from `misc/scripts/oit_intervals_why.cpp`: the padded boxes merge to 4 intervals and the first runs 0.01-6.62 m with linearization k = 661, so the log curve packs the slices near the near plane. Fragments at 1.60 m and 4.15 m land on slices 25.0 and 29.7, five slices apart across 2.5 m. The rule that gives every interval an equal share of the slices does not weigh how much geometry each holds.
- So the froxel's real score depends on how tight the instance AABBs are. Measure that on the real scenes (stack, hair) before trusting either number.
- Decision: build the froxel with the engine's interval path. Not tested on a GPU.

## Iteration 6: the packed scan (`oit_scan_packed_column`, `misc/scripts/oit_scan_packed_check.cpp`)

The kept atlas packs four slices into one RGBA texel (tile z / 4, channel z % 4), so the one-value-per-texel scan of iteration 1 does not apply.
The packed scan is three kinds of fragment pass, each reading only the previous buffer and writing only its own texel:
local (in-texel prefix over the four channels), tiles (Hillis-Steele over the tile totals in channel 3), carry (add the left neighbour's channel 3).

- Matches the sequential prefix sum on 1575 cases (21 slice counts from 1 to 256, three densities), worst error 3.05e-05 against 1e-03.
- Pass count equals the unpacked scan from 4 slices up: 128 slices take 7 passes both ways (1 local + 5 tiles + 1 carry); 1 slice takes 2 against 0.
- Planted faults, each caught on every trial: carry skipped 210/210, carry reads the tile's own total 210/210, tile scan one pass short 210/210.
- The fault harness with no fault equals the real function, so the faults are injected into the same sequence.
- A first fault set scored 150/210 on the short tile scan. The misses were slice counts 33 and 129, where the last tile holds one slice: channel 3 of the last tile is only read as the carry of a next tile that does not exist, so that fault cannot change an output. Those counts were replaced by 36 and 132 and the reason is written in the check.
- Nothing here runs on a GPU. The frontier numbers of iteration 2 are unchanged.

## Iteration 7: the scan models in the engine's test runner

`tests/servers/rendering/test_oit.cpp` gains five `[OIT] scan:` cases: the unpacked pass sequence against the sequential sum (14 slice counts), the packed sequence the same (20 counts, partial last tiles included), the pass counts (128 slices take 7 both ways), and two controls that must fail (packed scan without its carry pass; tile scan one pass short on a full last tile).

- Built the macOS editor with `tests=yes` (333 s). `misc/scripts/oit_tests.sh`: 38 OIT cases matched, 38 passed, 5928 assertions. The five scan cases alone: 278 assertions.
- Planted fault in `oit_scan_math.h` (the carry reads the tile's own total, not its left neighbour's), rebuilt, reran: the `[OIT] scan:` cases fail. The header was restored from a saved copy and checked against the commit (empty diff), then a clean rebuild passed all 38.
- First compile failed: the engine's `LocalVector` copy constructor is explicit, so `LocalVector<float> got = col;` does not compile; written `got(col)`.
- Before this, `oit_tests.sh` had never run against this branch: the editor in `zone-web` was built with `tests=no`, so the engine test suite had no coverage of the scan models.

## Iteration 8: the packed scan as GLSL, run on a GPU

`drivers/gles3/shaders/effects/oit_scan.glsl` has three modes (`mode_local`, `mode_tiles`, `mode_carry`), one per pass of `oit_scan_packed_*`. Tiles of a column sit side by side in x, `grid_width` apart.

- `glslangValidator -S frag` accepts all three modes as GLSL ES 3.00; a copy with a planted type error (`v.w += v.zz`) is rejected.
- `misc/scripts/oit_scan_gl/index.html` runs the passes in headed Firefox 160, WebGL2 over ANGLE Metal on an Apple M1: 11 cases (4 to 200 slices, odd and even tile counts, grids up to 7 wide), worst error 0 against the CPU prefix sum, no GL error. Pass counts equal the model's (128 slices: 7). The error is exactly 0 because the inputs are multiples of 1/65536 whose sums stay under 2^24.
- Control: the carry reading its own tile (`p.x` instead of `p.x - grid_width`) is off by 2.3 to 3.1 in all three trials, so the harness can fail.
- Not done: the shader is not in `drivers/gles3/shaders/effects/SCsub`, nothing in `drivers/gles3` calls it, and it has not run inside the engine.

## Iteration 9: the splat step, two designs in WebGL2

`misc/scripts/oit_splat_gl/index.html` runs both designs of open problem 2 in Firefox 160 (WebGL2 over ANGLE Metal, Apple M1; 8 draw buffers, RGBA32F blend works) against a CPU answer: 2251 fragments over 2168 froxels.

- A, slice groups at froxel resolution: matches. 32 froxels differ from the reference, all within 0.01 slice of a boundary where a GPU's float depth can round across it. 160 draws (40 quads x 4 passes of 8 attachments).
- B, point sprites: matches with worst error 9.5e-08, 1 draw of 2251 points.
- Control: B shifted three slices fails, 4192 unexplained froxels (worst 0.96).
- No GL error.
- A first run of A had 4230 wrong froxels. The cause was my harness: the depth uniform was uploaded as (z0, z0 + dz) where the shader expects (z0, dz). A second cause of doubt, the 32 boundary froxels, was measured and is rounding (farthest from an edge: 1.2e-3 slice), so the tolerance is written into the page and applied to both designs.

What this does not decide. B's samples come from the CPU reference, so B cannot yet take a real mesh: GLES3 has no way to read an arbitrary Godot surface as a point list in the vertex stage without a sample-generation pass that does not exist. A takes the surface's own fragments, which is what the transparent pass already draws, so A is the design the engine can use without new geometry plumbing. The timings (A 2.9 ms, B 0.9 ms) are for 40 quads at 40x24 and say nothing about headset size.
- Next: wire A into `mode_oit_splat` in `scene.glsl`, or measure A at headset froxel size first.

## Iteration 10: double-single slice maths for the splat

The 32 froxels that differed in iteration 9 were fragments within 0.01 slice of a boundary. WebGL2 has no float64, so the slice maths now carries each value as a hi + lo float32 pair (about 48 bits, "2x f32 is an f64"): the 127 slice boundaries are stored as pairs and compared lexicographically, and the quad's depth is computed in the fragment shader from `gl_FragCoord.x` as a + b x with a Dekker split (GLSL ES 3.00 has no fma). Quad inputs are rounded to float32 so the GPU and the double-precision reference start from the same numbers. Firefox 160, ANGLE Metal, M1:

| scene | edge | `log` | float32 table | double-single | double-single, lo zeroed |
| --- | --- | --- | --- | --- | --- |
| 40 random quads | 0.01 | 32 | 32 | 0 | 0 |
| 40 flat quads on rounded boundaries | 1e-9 | 552 | 336 | 0 | 336 |

Differing froxels in variant A; variant B (point sprites, depth passed as a hi/lo vertex attribute pair) gives the same numbers except 0 on the random scene in all modes. The shifted-output control fails in every run. No GL error.

- The random scene alone could not show the lo part earning its place: with the lo parts zeroed it also scored 0, because almost no froxel there sits within 1e-6 slice of a boundary. The stress scene is what separates them (0 against 336).
- The earlier 32 are not a `log` against table effect: both give 32. They disappear with the exact depth and the paired boundaries together.
- Two harness defects on the way: the tolerance read a variable before it was declared, so the first batch of runs threw at startup and measured nothing; and `slice_of_ds` was missing from the `log` shader, so that run failed to compile until a stub was added.
- Cost not measured. The double-single depth is about 30 extra ALU ops per fragment and 14 more compares per fragment in the search; both are small next to the 4 passes of 8 attachments, but I have not timed them.
- Variant B still takes its samples from the reference, as before.
