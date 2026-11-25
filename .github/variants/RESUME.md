# Resume note: the variants matrix, the sqlite client stubs, the vendoring report

Written 2026-11-25 so another person or computer can pick this work up. Nothing here is abandoned. Under RFD66606-2490 the beta (RFD66606-2360, the sight gate of RFD66606-2436) comes first, so the parts marked "waits" stay parked until it is cut.

## Done

- **sqlite on every variant, client by stubs.** `entities-godot` `dev/sqlite` at `1a606f8f`: `modules/sqlite/fdb/fdb_c.sigs` lists the FoundationDB client entry points the VFS calls, `SCsub` generates a dlsym table from it with `thirdparty/generate_stubs` (the `native_media` pattern), and `fdb/fdb_client.cpp` opens `libfdb_c` at run time, beside the binary first. No client package, `fdb_c.h` or `libfdb_c` is needed to build. `fdb/win_compat/dlfcn.h` maps `dlopen` onto `LoadLibrary` for MinGW.
- **Variants workflow.** `.github/workflows/variants.yml` and this directory (`variants.json`, `expand_variants.sh`): 144 jobs, every official Godot variant at single and double precision. Linux, Windows, Web and Android build in godotengine/build-containers images (pinned commit `9d1304ed`, scripts `b7436b1f`); macOS, iOS and visionOS build on macos-26. Not built: mono, dotnet, tvOS, Android Gradle packaging.
- **Inventory.** One `guest-builds` entry for the Linux binary: chibifire-smoke-water/datasource-inventory PR #9, merged to main with PR #8. A private static browser: Hugging Face Space `chibifire/inventory` (its own git repo; the data is a snapshot).

## Verified, and not

- Linux x86_64 `template_release` built with no client installed; `readelf -d` lists only libm, libc, ld-linux; `--version` runs.
- Stubs tested standalone: a missing library returns false without a crash; a stand-in library dispatches.
- **Not verified:** Windows (MinGW dlfcn shim, weak symbols), macOS, any CI run of `variants.yml`, `SQLite.open` against a real `libfdb_c`, a NixOS flake.
- The assembler run of 2026-11-25 pushed tag `v0.0.0-20261125142020-e1ed19844b5e` to entities-zone; its `double.yml` run is https://github.com/V-Sekai-fire/entities-zone/actions/runs/45654602261. Its Windows job is the first MinGW test of the shim. The assembler then failed pushing `dev/latest-tag`: entities-zone's rules refuse a force-push and require a pull request and the merge queue. That conflict is open.

## Next

1. Read the Windows result of the run above; fix the shim as new commits on `dev/sqlite`.
2. Run the FDB-backed `SQLite.open` with a real `libfdb_c` beside the binary on Linux and Windows.
3. NixOS: a flake must put `libfdb_c` beside the binary or on the loader path (`LD_LIBRARY_PATH`).
4. Done: the test branch `test/sqlite-sigs-win` is deleted. No test release existed.
5. Dispatch `variants.yml` on entities-zone with `only=<job name substring>` (it is not registered on entities-godot, where Actions for forks are off, RFD66606-2387).
6. Waits (not on the beta path): the variants rows beyond the beta targets, the vendoring changes below, the inventory Space.

## Check it

    ./.github/variants/expand_variants.sh | jq '.include | length'      # 144
    scons platform=linuxbsd target=template_release arch=x86_64 module_sqlite_enabled=yes production=no
    readelf -d bin/godot.linuxbsd.template_release.x86_64 | grep NEEDED   # no libfdb_c

## Vendoring report (investigation only; no change made)

- **Already vendored:** engine `thirdparty/` (136 MB) and `modules/sqlite/thirdparty/foundationdb` (46 MB, the whole FDB source, built only by the standalone CMake store, not by SCons). The stubs vendor only the client's declarations (`fdb/fdb_c.h`, about 80 lines).
- **Fetched at build time:** the build-containers images pull from about 7 hosts (godotengine/buildroot SDKs for 4 arches, llvm-mingw, emsdk, Android SDK and NDK, Fedora packages, swiftly, darwin-tools, pbzx); AccessKit through `install_accesskit.py`; glslc through setup-pixi; actions are pinned by tag, not by commit.
- **Three savings:** (1) build the four container images once, publish to GHCR and pull by digest, instead of every job rebuilding them from cache: less runner time, reproducible; cost is one image workflow, image storage in GB, and a bump when build-containers moves. (2) Vendor AccessKit and glslc: small, removes network drift. (3) Pin actions by commit hash.
- **Not worth it:** vendoring the 46 MB FDB source to build `libfdb_c` per variant. It needs Boost and flow CMake per arch and saves nothing, since the stubs dlopen a binary anyway; at most a static client for the two beta hosts. Apple SDKs cannot be vendored (licence); macOS rows stay on runners.
- Cache hit rates and minutes are not measured, because the workflow has never run. Sizes of AccessKit and the toolchains are not measured.

## Machine-specific (this desk, WSL)

- Tokens: `bao-mint-github --owner <org> --token-only` and `bao-hf-token` (both talk to the local Bao). Nothing else holds a credential.
- Scratch clones under the session's scratchpad (`ez`, `eg`, `dr`, `inv`) are disposable; everything of value is on the branches below.
- No MinGW, no flyctl here, so Windows and Fly could not be tested or queried.

## Files not in git

- `godot.linuxbsd.template_release.x86_64`, 126609048 bytes, sha256 `fdf1c35b9595d0498187dd9c87c49ba8100c251030e7b6e7bb924512cde42d70`. Rebuild: entities-zone `2bb0aa62247c` plus the `modules/sqlite` change of `dev/sqlite` `1a606f8f`, SCons 4.11.1, gcc 13.3.0, the scons line under "Check it", about 27 minutes on 8 cores. A rebuild may not give the same hash (not tested).
- The Space's `data/*.json`: rebuild with `elixir tools/load.exs inventory.db` in datasource-inventory, then `python3 tools/export.py inventory.db` (the script is in the Space repo, `tools/export.py`).

## Branches

- `entities-godot`: `dev/sqlite` (`1a606f8f`), `dev/double-release` (`05ba06d`), this note on `dev/variants-resume`, (the test branch is deleted).
- `datasource-inventory`: entry merged to main (PR #9).
- Hugging Face: Space `chibifire/inventory`.
