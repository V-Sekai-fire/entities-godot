# Python actor compiler and vexillographer against the 7.3 C# tools

Oracle: the fork's own C# compiler built with mcs under mono, run over every `*.actor.cpp` and `*.actor.h` (501 files). Recipes: `flow/actorcompiler_py/containers/` (`Containerfile.parity` builds the image the README names, `fdb-parity`).

| State | Runs | Identical | Different | Failed |
| --- | --- | --- | --- | --- |
| Port as taken from upstream 41ffc89f (harness then also ran `--disable-diagnostics`) | 1503 | 156 | 1347 | 0 |
| After the patch (`--generate-probes` and none) | 1002 | 1002 | 0 | 0 |

The first row is the baseline that shows the harness can fail. The two rows use different flag sets, so compare the identical-to-different split, not the totals.

Vexillographer (`fdbclient/vexillographer/parity.sh`): c 45896 bytes, python 39229, ruby 38002, cpp.h 43185, cpp.cpp 57797 are byte-identical. Before the patch c, python, ruby and cpp.h differed. java is not ported and refuses.

Build check (`containers/flowbuild.sh`, aarch64 Linux, clang, `FDB_USE_CSHARP_TOOLS=OFF`): `libflow.a` 27,403,418 bytes, 97 of 97 steps, 21 actor files through `actorcompiler_py`. One compiler warning in the log, not read. Not run on Windows or x86_64.
