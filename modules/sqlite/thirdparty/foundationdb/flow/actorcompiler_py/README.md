# actorcompiler_py

A Python port of the Flow actor compiler. It reads `X.actor.cpp` or `X.actor.h` and writes `X.actor.g.cpp` or `X.actor.g.h`, so the build needs no C# toolchain.

## Origin and what was changed

The files come from upstream FoundationDB commit 41ffc89f, the last tree that carried a Python port (upstream removed it, with the C# compiler, in 407ae2be). That port was written against a later C# compiler than this fork's 7.3 one, so it was adjusted to match the 7.3 C# compiler in `flow/actorcompiler/`:

- removed the `ActiveActorHelper` / `__actorIdentifier` members, the `WITH_ACAC` block, the `.uid` sidecar file and `SWIFT_ACTOR`;
- the wait state is the plain integer the 7.3 `flow.h` uses (`-1` cancelled, `> 0` waiting), not `ACTOR_WAIT_STATE_*`;
- `waitNext` declares `FutureStream<T> name = expr;`;
- added `DESCR` blocks (`DescrCompiler`, `parse_descr*`), which 7.3 still has;
- `destroy()` and the closing of the anonymous namespace are written as the 7.3 compiler writes them, including its literal `{{ }}` in the `void` case.

## Check

`parity.sh` is the oracle. It builds the fork's C# compiler with `mcs`, runs both compilers over every `*.actor.cpp` and `*.actor.h` in the tree (with and without `--generate-probes`), and fails on any byte difference or any difference in exit code. It needs mono and python3, so it runs in a container:

    podman run --rm -v "$PWD":/src:ro -v /some/work:/work localhost/fdb-parity bash /src/flow/actorcompiler_py/parity.sh

`FILES='a.actor.cpp b.actor.h'` limits the run. A change to the C# compiler, or an upstream bump, needs this to pass again.

## Run

    PYTHONPATH=flow python -m actorcompiler_py <input> <output> [--generate-probes] [--disable-diagnostics]

Not checked: `--disable-diagnostics` (it only changes the error policy), and that the generated code compiles under clang-cl.

Licence: Apache-2.0, as the rest of this tree.
