// SPDX-License-Identifier: MIT
#include <cstdio>
#include <cstdlib>
#include "predictive_bvh.h"

static int fail(const char *what, long row) {
  printf("MORTON %s at row %ld\nRESULT: FAIL\n", what, row);
  return 1;
}

int main(int argc, char **argv) {
  if (argc != 2) { printf("RESULT: ERROR give the golden file\n"); return 2; }
  FILE *f = fopen(argv[1], "r");
  if (!f) { printf("RESULT: ERROR cannot read %s\n", argv[1]); return 2; }
  const Aabb scene = {0, 1024000, 0, 2048000, -512000, 512000};
  const int64_t ux = 1000, uy = 2000, uz = 1000;
  unsigned x, y, z, want;
  long rows = 0;
  while (fscanf(f, "%u\t%u\t%u\t%u", &x, &y, &z, &want) == 4) {
    uint32_t code = morton3d(x, y, z), rx, ry, rz;
    if (code != want) return fail("the code differs from Lean", rows);
    morton3d_inverse(code, &rx, &ry, &rz);
    if (rx != x || ry != y || rz != z) return fail("the inverse differs", rows);
    Aabb box = {scene.min_x + x * ux, scene.min_x + (x + 1) * ux, scene.min_y + y * uy,
                scene.min_y + (y + 1) * uy, scene.min_z + z * uz, scene.min_z + (z + 1) * uz};
    if (morton_of_aabb(&box, &scene) != want) return fail("the code of a box differs", rows);
    for (uint32_t d = 0; d <= 10; d++) {
      uint32_t s = 10 - d;
      Aabb c = morton_cell_of(code, d, &scene);
      Aabb o = morton_cell_of((code >> (3 * s)) << (3 * s), d, &scene);
      bool octree = c.min_x == scene.min_x + (int64_t)((x >> s) << s) * ux && c.max_x - c.min_x == ((int64_t)1 << s) * ux &&
                    c.min_y == scene.min_y + (int64_t)((y >> s) << s) * uy && c.max_y - c.min_y == ((int64_t)1 << s) * uy &&
                    c.min_z == scene.min_z + (int64_t)((z >> s) << s) * uz && c.max_z - c.min_z == ((int64_t)1 << s) * uz;
      bool same = c.min_x == o.min_x && c.min_y == o.min_y && c.min_z == o.min_z && c.max_x == o.max_x;
      if (!octree || !same) return fail("a prefix is not its octree cell", rows);
    }
    rows++;
  }
  fclose(f);
  if (rows < 500) return fail("too few rows", rows);
  printf("MORTON rows=%ld depths=11\nRESULT: PASS\n", rows);
  return 0;
}
