# oit_splat_gl

Two WebGL2 designs for the splat step, against one CPU answer (40 translucent quads, 40x24 froxels, 128 slices, four slices to a texel).

- **A, slice groups:** the quads are rasterised at froxel resolution, as RD Raster mode does. Each fragment takes its slice from its own depth, writes channel `z % 4` of tile `z / 4`, and discards the tiles that are not in the pass. Eight tiles go out per pass as eight colour attachments, so 128 slices take 4 passes.
- **B, point sprites:** one GL point per sample lands on its atlas texel. The samples here come from the CPU reference, so B shows the placement and blending only, not how a real mesh would produce them.

Serve the repository root and open `misc/scripts/oit_splat_gl/index.html` in a headed browser (see `../oit_scan_gl/README.md`). The page prints `SPLAT_RESULT`. A froxel counts as wrong only when it differs from the reference and no fragment lies within 0.01 slice of its boundary. The control shifts B's output by three slices and must fail.

Slice maths, chosen by the query string: default is double-single (every boundary and the quad's depth carried as a hi + lo float32 pair, compared lexicographically); `?table` is a float32 boundary table; `?log` is the shader's `log()`. `?nolo` zeroes the lo parts, as a control. `?stress` swaps in 40 flat quads whose depth is a float32-rounded slice boundary. `?edge=1e-9` sets the tolerance in slices.
