# tint

Subset of Dawn's Tint (WGSL compiler), used by the WebGPU rendering
device driver for SPIR-V → WGSL shader translation.

- Upstream: https://github.com/floooh/tint-extract
- Version: `308db188c0110349ddbde72011203e9b8cbda337` (see `TAG`)
- Extracted from google/dawn @ `b4219e7e4f0f0bae6239164e786bd7136af244aa` (see `dawn.ref`)
- License: BSD-3-Clause (Dawn's)

Contains only the SPIR-V reader and WGSL writer. GLSL/HLSL/MSL emitters,
WGSL reader, LSP, and platform command-line utilities are stripped by
`floooh/tint-extract`'s own `release.sh`. Refreshed by `update-tint.sh`.
