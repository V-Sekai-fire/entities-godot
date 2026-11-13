# predictive_bvh

The generated header of the predictive bounding-volume hierarchy, with the 30-bit Morton code the
tree sorts by. Nothing here is written by hand and nothing here is edited.

## Source

`V-Sekai-fire/contract-spatial-oracle` at commit `032a449c9da84fe8df91f79336cb8cdd313c0082`. The Lean proofs and the generator
(`lake exe bvh-codegen`) live there and only there. This directory used to hold a copy of the Lean
sources, a generated Rust file, and, in `proofs/` at the top of the tree, a second copy taken by
git-subrepo. Both copies were older than their source and are removed.

| File | From | sha256 |
| --- | --- | --- |
| `predictive_bvh.h` | `predictive_bvh.h` | `e02e62cd2b75e5d7b51ae9e04f14920b96a869043afb7e452470d077841b75c7` |
| `morton_golden.tsv` | `testdata/morton_golden.tsv` | `6e1a0ea27fa231e5d1a31f6af270542e8e3d03f908600f208fdef5d6e4046591` |
| `morton_check.cpp` | `tests/morton_check.cpp` | |

To move to a newer generator, copy the three files from one commit and change the commit and the
hashes here.

## The curve

The header defines `morton3d`, `morton3d_inverse`, `morton_of_aabb` and `morton_cell_of`.
`core/math/predictive_bvh_adapter.h` defined those four by hand under the name of another curve
until this change; that encoder did not have the locality its name promised, and Lean's correction
never reached it. `docs/morton.md` of the source repository has the account. The tree's sort key
is a code and is named `code`; every value differs from the old encoder's.

## The check

```sh
sh misc/scripts/morton_gate.sh
```

It compiles `morton_check.cpp` against the header and holds every row of the golden file to
Lean's value, the inverse, the code of a box, and the octree cell of every prefix depth. Before
that it must see a header with the old axis order fail and a golden file with one altered code
fail. It also fails when a line of `core/`, `tests/` or this directory names the old curve, after
the lint has found a planted name.
