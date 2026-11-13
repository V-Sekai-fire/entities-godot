# oit_scan_gl

Runs `drivers/gles3/shaders/effects/oit_scan.glsl` (three passes, four slices per RGBA32F texel) in a browser's WebGL2 and compares every column with a CPU prefix sum. A control with the carry reading its own tile must fail.

    python3 -m http.server 8765        # from the repository root, with COOP/COEP not needed
    # open http://127.0.0.1:8765/misc/scripts/oit_scan_gl/index.html in a headed browser

The page prints `SCAN_RESULT` as JSON and posts it to `/log` if the server accepts POST. Needs `EXT_color_buffer_float`.
