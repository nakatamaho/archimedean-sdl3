# Status

Current milestone: M11 — complete.

## Dependency baseline

- SDL target release: 3.4.16
- nlohmann/json target release: 3.12.0
- SageMath version: 10.9 (Conda-forge environment used for M2)
- PyNormaliz/Normaliz version: PyNormaliz 2.23 / Normaliz 3.11.0

## Completed work

- M0 — repository bootstrap and documentation scaffold committed.
- M1 — vendored dependency and CMake/headless executable baseline committed.
- M2 — SageMath generator for all 13 solids.
- M3 — independent CPython validator and canonical JSON data committed.
- M4 — SDL-independent C++ model loader, topology validation, and math library.
- M5 — SDL3 CPU-side filled polygon renderer with culling and depth sorting.
- M6 — flat Lambert shading, polygon-size palette, and testable lighting path.
- M7 — keyboard controls, dynamic solid cycling, view state, wireframe overlay, and title status.
- M8 — MinGW-w64 cross-build with statically linked SDL3 and Wine compatibility checks.
- M9 — Ubuntu and Windows/MSYS2 GitHub Actions validation jobs.
- M10 — fresh-clone release QA, deterministic regeneration check, release checklist, and final documentation.
- M11 — v1.3 X11-ico-style motion, runtime mode toggle, and in-window help overlay.

## Evidence

Authentication and repository creation were verified on 2026-09-21.

Exact commands and observed results:

```text
$ gh auth status
github.com
  ✓ Logged in to github.com account nakatamaho
  - Active account: true
  - Git operations protocol: https
  - Token scopes: 'gist', 'read:org', 'repo', 'workflow'

$ gh repo view archimedean-sdl3 --json nameWithOwner,url,visibility,defaultBranchRef
GraphQL: Could not resolve to a Repository with the name 'nakatamaho/archimedean-sdl3'. (repository)
```

The absence check preceded creation. The exact creation command was:

```sh
gh repo create archimedean-sdl3 --public --clone --license bsd-2-clause --description "SageMath-generated Archimedean solids with an SDL3 interactive shaded viewer"
```

Observed creation output:

```text
https://github.com/nakatamaho/archimedean-sdl3
Cloning into 'archimedean-sdl3'...
```

Post-creation repository evidence:

```text
$ git status --short --branch
## main...origin/main

$ git remote -v
origin  https://github.com/nakatamaho/archimedean-sdl3.git (fetch)
origin  https://github.com/nakatamaho/archimedean-sdl3.git (push)

$ gh repo view --json nameWithOwner,url,visibility,defaultBranchRef
{"defaultBranchRef":{"name":"main"},"nameWithOwner":"nakatamaho/archimedean-sdl3","url":"https://github.com/nakatamaho/archimedean-sdl3","visibility":"PUBLIC"}
```

The initial GitHub-generated commit contained only the BSD-2-Clause `LICENSE`.
The documentation scaffold was added locally and is committed in the M0
scaffold commit. No implementation code, dependencies, generated JSON, or CI
was added.

Local commit evidence:

```sh
git add .gitignore AGENTS.md CODEX.md README.md docs
git commit -m "docs: bootstrap repository"
```

Observed commit result:

```text
[main 7fd97d1] docs: bootstrap repository
 8 files changed, 905 insertions(+)
 create mode 100644 .gitignore
 create mode 100644 AGENTS.md
 create mode 100644 CODEX.md
 create mode 100644 README.md
 create mode 100644 docs/handoff.md
 create mode 100644 docs/milestones.md
 create mode 100644 docs/spec.md
 create mode 100644 docs/status.md
```

Immediately after that commit, `git status --short --branch` reported
`## main...origin/main [ahead 1]`, and `git log -1 --oneline` reported
`7fd97d1 docs: bootstrap repository`.

Publication evidence for the documentation and evidence commits:

```sh
git push origin main
```

```text
To https://github.com/nakatamaho/archimedean-sdl3.git
   de4385c..ba144d9  main -> main

## M1 evidence

Pinned submodules:

```text
$ git submodule status --recursive
 fa2c02bb6e21974a89ea9824bc53c9932abe5f9c external/SDL (release-3.4.16)
 55f93686c01528224f448c19128836e7df245f72 external/json (v3.12.0)
```

The vendored CMake path defines `archview_core`, `archview_renderer`,
`archimedean_viewer`, and `archview_tests`, plus the optional
`ARCHVIEW_USE_SYSTEM_DEPS` mode and
`cmake/mingw-w64-x86_64.cmake`.

Linux configure/build/test commands:

```sh
cmake --fresh -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/archimedean_viewer --help
./build/archimedean_viewer --selftest
./build/archview_tests
```

Observed results:

```text
-- Configuring done
-- Generating done
-- Build files have been written to: .../build
100% tests passed, 0 tests failed out of 1
PASS: M1 headless executable skeleton
PASS: archview_tests
```

The build and test commands exited successfully. The build used vendored SDL
and nlohmann/json. The system-dependency mode was not run because this host
does not provide system SDL3/json packages. The MinGW toolchain file exists;
cross-build evidence is deferred to M8, and no native Windows runtime claim is
made.

M1 implementation commit:

```text
[main 92b40c2] build: establish CMake and SDL baseline
 15 files changed, 302 insertions(+)
```

## M2 evidence

M2 adds the SageMath constructors and deterministic canonicalization without
committing the JSON artifact yet. The generator uses Sage's
`polytopes` constructors for all 13 solids, including
`polytopes.snub_dodecahedron(base_ring=AA, backend="normaliz")`. It sorts
vertices by normalized coordinates, cyclically orders each face in its plane,
forces outward winding, and preserves polygon faces.

The Sage environment used for execution is Conda-forge SageMath 10.9 with
PyNormaliz 2.23 and Normaliz 3.11.0. The exact constructor probe reported all
expected f-vectors, including:

```text
truncated_tetrahedron (1, 12, 18, 8, 1)
...
snub_dodecahedron (1, 60, 150, 92, 1)
```

The generator command executed in that environment was:

```sh
/tmp/codex-micromamba/bin/micromamba run -p /tmp/codex-sage python tools/generate_archimedean.py --output data/archimedean.json
```

It reported:

```text
generated 13 solids at data/archimedean.json
```

The installed Conda-forge Sage launcher is the Sage 10.9 CLI but does not
implement the legacy `sage -python` option; invoking that spelling produced
`sage: error: unrecognized arguments: --output data/archimedean.json`. The
tested equivalent runs the generator with the Sage environment's Python,
which imports `sage.all` and executes the Sage constructors. This launcher
compatibility detail is recorded rather than claimed as an exact
`sage -python` pass.

Determinism test:

```sh
/tmp/codex-micromamba/bin/micromamba run -p /tmp/codex-sage python tools/generate_archimedean.py --output <temporary>/one.json
/tmp/codex-micromamba/bin/micromamba run -p /tmp/codex-sage python tools/generate_archimedean.py --output <temporary>/two.json
cmp -s <temporary>/one.json <temporary>/two.json
```

Observed result:

```text
PASS: complete generated JSON is byte-identical
e616a715fc32833085afec33b02c70aafe85649b80f8768e5e8d88a397547f09  one.json
e616a715fc32833085afec33b02c70aafe85649b80f8768e5e8d88a397547f09  two.json
solids 13 sage 10.9
```

The generator's validation passed for every solid: expected V/E/F and
histograms, Euler characteristic two, two-face edge incidence, distinct
face vertices, in-range indices, no duplicate face edges, planarity at
`1e-9 * characteristic_radius`, normalized mean edge length one with
`1e-8` relative spread, finite coordinates, and outward winding.

M2 implementation commit:

```text
[main 61b3e5d] feat: add SageMath Archimedean generator
 2 files changed, 461 insertions(+)
```

## M3 evidence

The independent validator in `tools/validate_archimedean.py` uses only
CPython's standard `json`, `math`, and `argparse` modules plus the pure
expected-combinatorics table. It rejects non-finite JSON constants, validates
the schema, all 13 solids, polygon topology, explicit edges, geometric
normalization, planarity, outward winding, and statistics.

The canonical artifact was regenerated from the SageMath generator and then
validated without Sage:

```sh
/tmp/codex-micromamba/bin/micromamba run -p /tmp/codex-sage python tools/generate_archimedean.py --output data/archimedean.json
python3 tools/validate_archimedean.py data/archimedean.json
python3 tools/test_validate_archimedean.py
```

Observed results:

```text
generated 13 solids at data/archimedean.json
PASS: validated 13 solids
PASS: rejected bad-index
PASS: rejected missing-manifold-edge
PASS: rejected wrong-count
PASS: rejected nan-coordinate
PASS: valid canonical document remains accepted
```

The four negative cases cover an invalid vertex index, missing explicit
manifold edge, incorrect statistics, and NaN-like JSON input. CTest also
executes the validator and negative suite:

```text
100% tests passed, 0 tests failed out of 4
```

The committed JSON contains exactly 13 solids and preserves polygon faces;
triangulation is not present in the data artifact. Python bytecode is ignored
and absent from the commit.

M3 implementation commit:

```text
[main 45e891f] feat: validate and commit canonical solid data
 6 files changed, 10253 insertions(+), 1 deletion(-)
```

## M4 evidence

The C++17 core now loads schema v1 through nlohmann/json without an SDL
dependency. It validates all 13 solids, explicit polygon faces and edges,
expected combinatorics, Euler characteristic, manifold edge incidence,
planarity, outward winding, centroid, finite coordinates, and normalized edge
lengths. The math library provides finite vectors, robust normalization,
dot/cross products, axis-angle `Mat3` rotation, and right-handed
perspective projection with positive view-space Z as forward.

The headless checks were:

```sh
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/archimedean_viewer --selftest --data data/archimedean.json
./build/archimedean_viewer --selftest --data missing.json
```

Observed results:

```text
100% tests passed, 0 tests failed out of 3
PASS: loaded 13 solids; selected truncated_icosahedron
error: cannot open JSON file: missing.json
```

The C++ tests cover representative vector normalization, axis-angle rotation,
projection and near-plane rejection, all committed solids, missing-solid
lookup, unsupported schema version, and invalid vertex index. The
`--selftest` path does not create an SDL window.

M4 implementation commit:

```text
[main b934514] feat: add validated model loader and math library
 9 files changed, 843 insertions(+), 33 deletions(-)
```

## M5 evidence

The SDL renderer creates a resizable window, uses elapsed
`SDL_GetPerformanceCounter()` time for arbitrary-axis rotation, transforms
the selected model into positive-Z view space, culls faces whose outward
normal points away from the camera, fan-triangulates only transient render
vertices, sorts visible faces by depth from far to near, perspective-projects
them, and submits the triangles through `SDL_RenderGeometry()`. Polygon
faces in JSON remain unchanged.

Headless regression commands:

```sh
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/archimedean_viewer --selftest --data data/archimedean.json
SDL_VIDEODRIVER=offscreen SDL_RENDER_DRIVER=software timeout 2s ./build/archimedean_viewer --data data/archimedean.json --width 320 --height 240
```

Observed results:

```text
100% tests passed, 0 tests failed out of 3
PASS: loaded 13 solids; selected truncated_icosahedron
offscreen process exit=124 after timeout; it stayed in the render loop
and emitted only host EGL permission warnings
```

The offscreen run proves startup and repeated frame execution but is not a
visual-display PASS. This host has no `DISPLAY` or `WAYLAND_DISPLAY`, so a
real interactive visual run remains deferred.

M5 implementation commit:

```text
[main 040dcf2] feat: add SDL3 polygon renderer
 3 files changed, 293 insertions(+), 4 deletions(-)
```

## M6 evidence

M6 adds a fixed palette keyed by polygon side count for triangles, squares,
pentagons, hexagons, octagons, and decagons. Each visible face uses its
view-space Newell normal and the fixed directional light with
`ambient=0.25` and `diffuse=0.75`:

```text
intensity = ambient + diffuse * max(0, dot(normal, light_direction))
```

The final RGB channels are clamped and alpha remains one. The renderer's
lighting flag selects the lit path or the unlit base palette; the interactive
`L` key is completed in M7. The headless renderer-color test checks palette
separation, full illumination, ambient-only back lighting, unlit behavior,
and alpha preservation.

Exact regression commands:

```sh
cmake --fresh -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/archimedean_viewer --selftest --data data/archimedean.json
SDL_VIDEODRIVER=offscreen SDL_RENDER_DRIVER=software timeout 2s ./build/archimedean_viewer --data data/archimedean.json --width 320 --height 240
```

Observed results:

```text
100% tests passed, 0 tests failed out of 3
PASS: loaded 13 solids; selected truncated_icosahedron
offscreen process exit=124; it stayed in the render loop
```

The offscreen run emitted host EGL permission warnings but no application
error. It verifies startup and repeated frame execution, not a visual-display
pass. This host still has no `DISPLAY` or `WAYLAND_DISPLAY`, so real visual
shading evidence remains deferred.

M6 implementation commit:

```text
[main 9614dd0] feat: add flat Lambert face shading
```

## M7 evidence

The renderer now translates every documented SDL key into an SDL-independent
`ViewerState` action. The state machine covers pause/resume, next/previous
solid with wraparound, X/Y/Z axis presets, normalized azimuth/elevation edits,
clamped signed speed in `[-360, 360]` degrees per second, reverse, orientation
reset, wireframe and Lambert toggles, zoom in/out, and Home view reset. Elapsed
performance-counter time advances orientation rather than frame count. The
window title is refreshed at 10 Hz with solid, speed, normalized axis, and
paused/running status. The camera distance and pixel size are recomputed after
resize; the default distance fits the largest normalized solid.

The state transition test exercises pause behavior, wraparound, axis
normalization, speed and zoom clamps, toggles, reset semantics, and elapsed
rotation without creating an SDL window.

Exact regression commands:

```sh
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/archimedean_viewer --help
./build/archimedean_viewer --selftest --data data/archimedean.json
SDL_VIDEODRIVER=offscreen SDL_RENDER_DRIVER=software timeout 2s ./build/archimedean_viewer --data data/archimedean.json --width 320 --height 240
```

Observed results:

```text
100% tests passed, 0 tests failed out of 3
help listed the existing command-line options successfully
PASS: loaded 13 solids; selected truncated_icosahedron
offscreen process exit=124; it stayed in the render loop
```

The offscreen run is startup/frame-loop evidence only. No physical display is
available on this host, so interactive keyboard and visual screenshot evidence
remains deferred.

M7 implementation commit:

```text
[main a2369a6] feat: add viewer controls and state machine
```

## M8 evidence

The host provides `x86_64-w64-mingw32-g++` GCC 13-win32 and
`x86_64-w64-mingw32-windres`. The CMake toolchain and MinGW configuration now
select vendored SDL3 static (`SDL_SHARED=OFF`, `SDL_STATIC=ON`) and pass
`-static -static-libgcc -static-libstdc++` to executable links. The viewer
therefore does not import or require SDL3, GCC, or C++ runtime DLLs. The
cross-build also produced the headless test executable.

Exact cross-build and artifact commands:

```sh
cmake --fresh -S . -B build-mingw-static -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake -DSDL_SHARED=OFF -DSDL_STATIC=ON
cmake --build build-mingw-static --parallel 2
file build-mingw-static/archimedean_viewer.exe build-mingw-static/archview_tests.exe
test -f build-mingw-static/archimedean_viewer.exe
test -f build-mingw-static/archview_tests.exe
test -f build-mingw-static/external/SDL/libSDL3.a
test ! -e build-mingw-static/SDL3.dll
if objdump -p build-mingw-static/archimedean_viewer.exe | grep -Fqi 'SDL3.dll'; then exit 1; else echo 'PASS: viewer has no SDL3.dll import'; fi
```

Observed results:

```text
PE32+ executable for MS Windows, x86-64: archimedean_viewer.exe
PE32+ executable for MS Windows, x86-64: archview_tests.exe
SDL_SHARED:BOOL=OFF
SDL_STATIC:BOOL=ON
PASS: static libSDL3.a present and viewer has no SDL3.dll import
```

The Windows binaries were additionally exercised under the host's Wine
compatibility layer:

```sh
WINEDEBUG=-all wine ./build-mingw-static/archimedean_viewer.exe --selftest --data data/archimedean.json
WINEDEBUG=-all wine ./build-mingw-static/archview_tests.exe
```

Both returned zero and printed their PASS lines. Wine is not a native Windows
runtime, so these are compatibility checks, not native Windows evidence. No
native Windows machine or Windows GUI session is available on this host;
native Windows `--selftest` and GUI runtime evidence remain deferred.

M8 documentation commit:

```text
MinGW compiler: x86_64-w64-mingw32-g++ (GCC 13-win32)
SDL3/runtime linkage: complete static (`SDL_SHARED=OFF`, `SDL_STATIC=ON`, `-static -static-libgcc -static-libstdc++`)
cross-build: PASS
native Windows runtime: DEFERRED
```

The post-change GitHub Actions run `35586772349` also passed both Ubuntu
GCC/Ninja and Windows MSYS2 UCRT64/Ninja jobs; the Windows job used the static
SDL3 configuration.

## Complete static MinGW follow-up

The original MinGW artifact was SDL3-static but still imported
`libgcc_s_seh-1.dll` and `libstdc++-6.dll`. The MinGW toolchain and top-level
CMake configuration now enforce `-static -static-libgcc -static-libstdc++`.
Only Windows system DLLs remain in the PE import table.

Exact clean verification commands:

```sh
cmake --fresh -S . -B build-mingw-complete-static -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake -DSDL_SHARED=OFF -DSDL_STATIC=ON
cmake --build build-mingw-complete-static --parallel 4
grep -E '^CMAKE_EXE_LINKER_FLAGS:|^SDL_(SHARED|STATIC):' build-mingw-complete-static/CMakeCache.txt
objdump -p build-mingw-complete-static/archimedean_viewer.exe | grep -i 'DLL Name'
test -f build-mingw-complete-static/external/SDL/libSDL3.a
if objdump -p build-mingw-complete-static/archimedean_viewer.exe | grep -Eiq 'SDL3\.dll|libgcc|libstdc\+\+|winpthread'; then exit 1; else echo 'PASS: no SDL3/GCC/C++/winpthread DLL imports'; fi
WINEDEBUG=-all wine ./build-mingw-complete-static/archimedean_viewer.exe --selftest --data data/archimedean.json
WINEDEBUG=-all wine ./build-mingw-complete-static/archview_tests.exe
```

Observed results:

```text
CMAKE_EXE_LINKER_FLAGS:STRING=-static -static-libgcc -static-libstdc++
SDL_SHARED:BOOL=OFF
SDL_STATIC:BOOL=ON
PASS: no SDL3/GCC/C++/winpthread DLL imports
PASS: loaded 13 solids; selected truncated_icosahedron
PASS: archview_tests
```

The remaining imports are Windows system DLLs such as `KERNEL32.dll`,
`USER32.dll`, and `msvcrt.dll`; they are operating-system dependencies rather
than redistributable SDL3 or MinGW runtime DLLs. Native Windows runtime and
display-backed GUI evidence remain deferred.

The implementation and documentation for this follow-up are committed as:

```text
c45c175 fix: fully statically link MinGW runtime
```

GitHub Actions run `35588366875` passed both Ubuntu GCC/Ninja and Windows MSYS2
UCRT64/Ninja. The Windows job also passed the complete-static import check for
both executables.

## Embedded model follow-up

The viewer now embeds the canonical `data/archimedean.json` during the normal
CMake build. The generated C++ source is created from the committed JSON by
`cmake/embed_json.cmake`; it is not a hand-copied mesh or a second geometry
source. The default viewer path loads this embedded text, while an explicit
`--data PATH` keeps the external-file override for development and validation.

Exact verification commands:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel 2
(cd /tmp && /home/docker/work/archimedean-sdl3-codex/archimedean-sdl3/build/archimedean_viewer --selftest)
ctest --test-dir build --output-on-failure
cmake --fresh -S . -B build-mingw-complete-static -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake -DSDL_SHARED=OFF -DSDL_STATIC=ON
cmake --build build-mingw-complete-static --parallel 4
(cd /tmp && WINEDEBUG=-all wine /home/docker/work/archimedean-sdl3-codex/archimedean-sdl3/build-mingw-complete-static/archimedean_viewer.exe --selftest)
(cd /tmp && WINEDEBUG=-all wine /home/docker/work/archimedean-sdl3-codex/archimedean-sdl3/build-mingw-complete-static/archview_tests.exe)
```

Observed results:

```text
PASS: loaded 13 solids; selected truncated_icosahedron
100% tests passed, 0 tests failed out of 3
PASS: loaded 13 solids; selected truncated_icosahedron
PASS: archview_tests
```

Both default self-tests passed from `/tmp`, proving that the executables do
not need a working-directory-relative JSON file. The generated source and
embedded model are build artifacts; the canonical source remains the SageMath
generated JSON committed under `data/`.

## macOS and 1.0.0 release follow-up

The project version is now `1.0.0`. On Apple platforms the viewer target is a
native `.app` bundle with SDL3 statically linked and the canonical JSON
embedded. The release workflow builds both supported hosted architectures:
`macos-15-intel` for x86_64 and `macos-15` for arm64. It combines the two
executables with `lipo` into one universal application archive.

The macOS implementation and workflow commits are:

```text
443764d feat: add macOS universal release workflow
ea472fe ci: use supported macOS runner labels
```

GitHub Actions run `35592330646` passed all four jobs:

```text
Ubuntu GCC/Ninja: PASS
Windows MSYS2 UCRT64/Ninja: PASS
macOS x86_64/CMake (`macos-15-intel`): PASS
macOS arm64/CMake (`macos-15`): PASS
```

Each macOS job configured with `-DSDL_SHARED=OFF -DSDL_STATIC=ON`, built the
`.app` bundle, ran CTest (including the embedded-data self-test), validated the
committed JSON, and ran the embedded-data self-test directly. The host has no
native macOS runtime, so this CI evidence is the native macOS evidence for the
release. The archive is intentionally unsigned and not notarized.

Release workflow run `35593240435` completed successfully. The public release
is [`v1.0.0`](https://github.com/nakatamaho/archimedean-sdl3/releases/tag/v1.0.0)
with asset
`archimedean-sdl3-v1.0.0-macos-universal.tar.gz`. Its verified SHA256 is:

```text
95d7ae49cd52290ff62036be0ea916dc28f392e68a94d910d45c7893a5dafb5b
```

The downloaded archive contains `ArchimedeanViewer.app`, `LICENSE`, and
`README.md`; `file` verifies the application executable as a Mach-O universal
binary containing x86_64 and arm64 slices.

## M9 evidence

`.github/workflows/ci.yml` now defines Ubuntu GCC/Ninja and Windows MSYS2
UCRT64/Ninja jobs. Both recursively check out the SDL3 and nlohmann/json
submodules, configure Release builds, build the viewer and headless tests, run
CTest, validate the committed JSON with CPython, and run viewer `--selftest`.
Neither job attempts SageMath regeneration or claims visual correctness.

The first pushed workflow run exposed a runner-image dependency gap: Ubuntu
SDL configuration stopped because neither X11 nor Wayland development headers
were installed. The Windows job passed. The workflow was corrected to install
the SDL Linux window-development packages before configuration.

Exact remote CI evidence:

```sh
gh run view 35579051126 --json status,conclusion,jobs
gh run view 35579502458 --json status,conclusion,jobs
```

Observed final result for run `35579502458`:

```text
Ubuntu GCC/Ninja: success
Windows MSYS2 UCRT64/Ninja: success
workflow conclusion: success
```

The earlier run `35579051126` is retained as a documented failed
configuration attempt; it was not relabeled as a pass. Headless CI remains
non-visual evidence.

M9 implementation commits:

```text
[main 32d38c9] ci: add Linux and Windows validation jobs
[main ba585d5] ci: install SDL Linux window dependencies
```

## M10 evidence

Release documentation is complete in `README.md` and
`docs/release_checklist.md`. The README documents the project purpose, all 13
solid IDs, Linux and Windows/MinGW build paths, SageMath provenance and
determinism procedure, every keyboard control, schema summary, screenshot
placeholder, license, and dependency versions.

Fresh recursive-clone QA used the remote `main` commit `c8be498`:

```sh
qa_tmp=$(mktemp -d)
git clone --recursive https://github.com/nakatamaho/archimedean-sdl3.git "$qa_tmp/repo"
cd "$qa_tmp/repo"
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-release --parallel 2
ctest --test-dir build-release --output-on-failure
python3 tools/validate_archimedean.py data/archimedean.json
./build-release/archimedean_viewer --selftest --data data/archimedean.json
```

The clone was clean, its submodules were SDL commit
`fa2c02bb6e21974a89ea9824bc53c9932abe5f9c` and json commit
`55f93686c01528224f448c19128836e7df245f72`, and the observed results were:

```text
100% tests passed, 0 tests failed out of 3
PASS: validated 13 solids
PASS: loaded 13 solids; selected truncated_icosahedron
PASS: fresh recursive clone Release QA
```

Fresh SageMath regeneration from that clone was repeated and compared:

```sh
gen_tmp=$(mktemp -d)
/tmp/codex-micromamba/bin/micromamba run -p /tmp/codex-sage python "$qa_tmp/repo/tools/generate_archimedean.py" --output "$gen_tmp/one.json"
/tmp/codex-micromamba/bin/micromamba run -p /tmp/codex-sage python "$qa_tmp/repo/tools/generate_archimedean.py" --output "$gen_tmp/two.json"
cmp -s "$gen_tmp/one.json" "$gen_tmp/two.json"
sha256sum "$gen_tmp/one.json" "$gen_tmp/two.json"
python3 "$qa_tmp/repo/tools/validate_archimedean.py" "$gen_tmp/one.json"
cmp -s "$gen_tmp/one.json" "$qa_tmp/repo/data/archimedean.json"
```

Observed result:

```text
generated 13 solids at both paths
ce314ef86167934e92a170f0a50dc9ad0afbbd7a7d796e1a762f342527927676  one.json
ce314ef86167934e92a170f0a50dc9ad0afbbd7a7d796e1a762f342527927676  two.json
PASS: validated 13 solids
PASS: fresh-clone generated JSON matches committed artifact byte-for-byte
```

The final all-solid smoke sweep selected all 13 IDs with `--selftest` and
started the offscreen render loop for every ID; each timed out at the expected
one-second observation boundary rather than exiting with an application error.
The final data audit reported these aggregate face sizes:

```text
solid_count=13
face_sizes=[(3, 200), (4, 108), (5, 48), (6, 60), (8, 12), (10, 24)]
```

The M10-triggered GitHub Actions run `35580125630` completed successfully for
both Ubuntu GCC/Ninja and Windows MSYS2 UCRT64/Ninja. This is headless CI
evidence, not visual evidence.

Final QA limitations are explicit: this host has empty `DISPLAY` and
`WAYLAND_DISPLAY`, so no real display-backed screenshot or interactive visual
PASS was possible. No native Windows machine/session is available, so native
Windows runtime evidence remains deferred; the MinGW cross-build and Wine
compatibility checks are not relabeled as native Windows evidence.

M10 implementation commit:

```text
[main c8be498] docs: finish release documentation
```

## v1.2 regular-solid extension and release evidence

The v1.2 extension adds the five Platonic solids, in this canonical order:
`tetrahedron`, `cube`, `octahedron`, `dodecahedron`, and `icosahedron`. The
existing 13 Archimedean solids follow them, for a total of 18 deterministic
regular-solid models. SageMath constructors are used for all five additions;
no hand-copied coordinate table was added.

Implementation and release-workflow commits in this follow-up are:

```text
34621f7 feat: add Platonic solids for v1.2
02c1dbe fix: upload release files without app directory
ebbf3c4 fix: make release checksums portable
```

The local SageMath 10.9 environment was used for deterministic regeneration:

```sh
tmp_dir=$(mktemp -d /tmp/archview-v12-gen.XXXXXX)
/tmp/codex-micromamba/bin/micromamba run -p /tmp/codex-sage python tools/generate_archimedean.py --output "$tmp_dir/one.json"
/tmp/codex-micromamba/bin/micromamba run -p /tmp/codex-sage python tools/generate_archimedean.py --output "$tmp_dir/two.json"
cmp -s "$tmp_dir/one.json" "$tmp_dir/two.json"
python3 tools/validate_archimedean.py "$tmp_dir/one.json"
cp "$tmp_dir/one.json" data/archimedean.json
sha256sum data/archimedean.json
```

Observed result:

```text
generated 18 solids at both paths
PASS: validated 18 solids
solid_count=18
data/archimedean.json SHA256: 29dea1fc97a104f55b5b3d9a6b3899e527a12fb0ceaf2a6e11a95d61fdf113f1
```

The five new combinatorial records are `(V,E,F)` = `(4,6,4)`, `(8,12,6)`,
`(6,12,8)`, `(20,30,12)`, and `(12,30,20)`, respectively. The aggregate
face-size audit for all 18 models is `[(3,232), (4,114), (5,60), (6,60),
(8,12), (10,24)]`.

The clean local Linux static-SDL3 build and tests were:

```sh
cmake -S . -B build-v12-linux-static -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DSDL_SHARED=OFF -DSDL_STATIC=ON
cmake --build build-v12-linux-static --parallel 4
ctest --test-dir build-v12-linux-static --output-on-failure
python3 tools/validate_archimedean.py data/archimedean.json
./build-v12-linux-static/archimedean_viewer --selftest
```

Observed result: `100% tests passed, 0 tests failed out of 4`, validator PASS,
and `PASS: loaded 18 solids`. `libSDL3.a` was present and `ldd` found no
dynamic SDL3 dependency. All 18 IDs were also selected successfully through
the headless self-test loop. A display-backed GUI run remains deferred because
this host has no `DISPLAY` or `WAYLAND_DISPLAY`.

The local MinGW complete-static build was:

```sh
cmake -S . -B build-v12-mingw-static -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake -DSDL_SHARED=OFF -DSDL_STATIC=ON
cmake --build build-v12-mingw-static --parallel 4
ctest --test-dir build-v12-mingw-static --output-on-failure -E 'archview_tests|archview_embedded_selftest'
python3 tools/validate_archimedean.py data/archimedean.json
WINEDEBUG=-all wine ./build-v12-mingw-static/archimedean_viewer.exe --selftest --solid dodecahedron
WINEDEBUG=-all wine ./build-v12-mingw-static/archview_tests.exe
```

The unfiltered cross-build `ctest` attempt was not counted as a pass because
CTest tried to execute PE files through the Linux shell. The two native-binary
tests passed when run directly under Wine, and the filtered CTest run passed
both CPython validator tests. `objdump` found no `SDL3.dll`, libgcc,
libstdc++, or winpthread imports in either PE binary; UCRT API and Windows
system DLL imports remain operating-system dependencies. Native Windows
runtime and GUI evidence remain deferred.

GitHub Actions CI run `35595902956` for `ebbf3c4` passed Ubuntu and Windows
validation. Release workflow run `35595910773` passed all five jobs:

```text
Build Linux x86_64 (static SDL3): PASS
Build MinGW x86_64 (fully static): PASS
Build macOS x86_64: PASS
Build macOS arm64: PASS
Publish multi-platform release: PASS
```

The public [`v1.2.0` release](https://github.com/nakatamaho/archimedean-sdl3/releases/tag/v1.2.0)
contains these verified assets:

```text
archimedean-sdl3-v1.2.0-linux-x86_64.tar.gz
  19ddfeb66e9d2517257f35d22d258674cfa423906f0a7214f5c54ed9f03831c9
archimedean-sdl3-v1.2.0-macos-universal.tar.gz
  546b34eac93741f0fa2f177fa4aa154bc70ba428a847404898c03b73b74b8aec
archimedean-sdl3-v1.2.0-mingw-x86_64.tar.gz
  de8a5ae8cb5f5f5b3066b99237f88a0c15884e1f6353de7410d3db542ede9f82
```

`SHA256SUMS` verifies all three archives. The macOS payload is a Mach-O
universal executable with x86_64 and arm64 slices. The Linux payload is an
ELF x86_64 executable with SDL3 statically linked; normal Linux system
libraries remain dynamic. The MinGW payload is a PE32+ x86_64 executable with
SDL3 and the GCC/C++ runtime statically linked. The release is not signed or
notarized. The v1.2.0 tag was created by the first successful publish at
`02c1dbe`; the later workflow-only checksum fix was applied to the published
assets without rewriting the tag.

## M11 / v1.3 evidence

M11 adds the requested X11 `ico`-style motion and restores the complete
keyboard documentation. The default animation now keeps a 3D orientation
matrix and composes equal elapsed-time X- and Y-axis increments, matching the
classic `ico` pattern while remaining frame-rate independent. `I` toggles
between that mode and the existing arbitrary-axis mode; selecting an axis with
`1`, `2`, `3`, or the arrow keys enters arbitrary-axis mode. `H` toggles an
SDL3-window help panel rendered with `SDL_RenderDebugText`; no SDL_ttf or font
asset was added. Key-repeat events do not repeatedly toggle the help panel.

Implementation commit:

```text
d2b3cf2 feat: add ico-style motion and in-window help
```

The local Linux verification used a fresh v1.3 build directory:

```sh
cmake -S . -B build-v13-linux-static -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DSDL_SHARED=OFF -DSDL_STATIC=ON
cmake --build build-v13-linux-static --parallel 4
ctest --test-dir build-v13-linux-static --output-on-failure
python3 tools/validate_archimedean.py data/archimedean.json
./build-v13-linux-static/archimedean_viewer --help
./build-v13-linux-static/archimedean_viewer --selftest
./build-v13-linux-static/archimedean_viewer --selftest --solid icosahedron
```

Observed result:

```text
100% tests passed, 0 tests failed out of 4
PASS: validated 18 solids
PASS: loaded 18 solids; selected truncated_icosahedron
PASS: loaded 18 solids; selected icosahedron
```

The SDL-independent tests additionally cover matrix composition, default
ico-mode orientation changes, pause behavior, axis-mode switching, help
visibility toggling, and Home reset. `git diff --check` passed. A dummy-video
runtime smoke test was also run:

```sh
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software timeout 2s ./build-v13-linux-static/archimedean_viewer --solid cube --width 320 --height 240
```

It remained running until the expected timeout (`exit_code=124`), indicating
that the SDL event/render loop started without a display. This is not visual
GUI evidence; the host still has no `DISPLAY` or `WAYLAND_DISPLAY`.

The v1.3 MinGW verification used:

```sh
cmake -S . -B build-v13-mingw-static -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake -DSDL_SHARED=OFF -DSDL_STATIC=ON
cmake --build build-v13-mingw-static --parallel 4
ctest --test-dir build-v13-mingw-static --output-on-failure -E 'archview_tests|archview_embedded_selftest'
python3 tools/validate_archimedean.py data/archimedean.json
WINEDEBUG=-all wine ./build-v13-mingw-static/archimedean_viewer.exe --selftest
WINEDEBUG=-all wine ./build-v13-mingw-static/archimedean_viewer.exe --selftest --solid icosahedron
WINEDEBUG=-all wine ./build-v13-mingw-static/archview_tests.exe
```

Observed result:

```text
PE32+ x86-64 viewer and test binaries
100% tests passed, 0 tests failed out of 2
PASS: validated 18 solids
PASS: loaded 18 solids; selected truncated_icosahedron
PASS: loaded 18 solids; selected icosahedron
PASS: archview_tests
```

`objdump -p` found no `SDL3.dll`, `libgcc`, `libstdc++`, or `winpthread`
imports in either PE binary. The filtered CTest invocation excludes PE tests
that cannot execute through the Linux host shell; direct Wine execution covers
those binaries. Native Windows runtime and visual GUI evidence remain
deferred as before.

GitHub Actions CI run `35600041065` passed for the pushed v1.3 changes:

```text
Ubuntu GCC/Ninja: PASS
Windows MSYS2 UCRT64/Ninja: PASS
macOS x86_64/CMake: PASS
macOS arm64/CMake: PASS
```

The run executed CTest, committed-JSON validation, and the headless viewer
self-test in every platform job. It provided no display-backed visual evidence.

## v1.3.0 release evidence

The release-preparation commit was `43f0f9b chore: prepare v1.3 release
workflow`, and the public release workflow run `35601204659` passed all five
jobs:

```text
Build Linux x86_64 (static SDL3): PASS
Build MinGW x86_64 (fully static): PASS
Build macOS x86_64: PASS
Build macOS arm64: PASS
Publish multi-platform release: PASS
```

The public [`v1.3.0 release`](https://github.com/nakatamaho/archimedean-sdl3/releases/tag/v1.3.0)
is not a draft or prerelease. Its tag points to `43f0f9b2708c3e91b2de24c8c8f373b538d091a8`
and contains the following verified assets:

```text
archimedean-sdl3-v1.3.0-linux-x86_64.tar.gz
  0cc92063b5723b37f80e615d15a36403bb816be437a06e4679193a7fb62ee4f6
archimedean-sdl3-v1.3.0-macos-universal.tar.gz
  c88925b45452c9458482a96210dfc67e003d9d503a7e48b107414a98081d6ae2
archimedean-sdl3-v1.3.0-mingw-x86_64.tar.gz
  2c8de666f0cc7c73963200ba1ff834e28a5799bebe26d5768592637cd477f315
```

The release was started and inspected with these commands:

```sh
gh workflow run release.yml --ref main -f release_tag=v1.3.0
gh run watch 35601204659 --exit-status
gh release view v1.3.0 --repo nakatamaho/archimedean-sdl3
git ls-remote origin refs/tags/v1.3.0
archive_tmp=$(mktemp -d)
gh release download v1.3.0 --repo nakatamaho/archimedean-sdl3 --dir "$archive_tmp"
cd "$archive_tmp"
sha256sum -c SHA256SUMS
tar -tzf archimedean-sdl3-v1.3.0-linux-x86_64.tar.gz
tar -tzf archimedean-sdl3-v1.3.0-macos-universal.tar.gz
tar -tzf archimedean-sdl3-v1.3.0-mingw-x86_64.tar.gz
mkdir linux mingw macos
tar -xzf archimedean-sdl3-v1.3.0-linux-x86_64.tar.gz -C linux
tar -xzf archimedean-sdl3-v1.3.0-mingw-x86_64.tar.gz -C mingw
tar -xzf archimedean-sdl3-v1.3.0-macos-universal.tar.gz -C macos
readelf -d linux/archimedean_viewer
objdump -p mingw/archimedean_viewer.exe
file macos/ArchimedeanViewer.app/Contents/MacOS/archimedean_viewer
```

`SHA256SUMS` downloaded from the release verifies all three archives. The
Linux archive contains an ELF x86_64 viewer with no dynamic SDL3 dependency;
normal Linux system libraries remain dynamic. The MinGW archive contains a
PE32+ x86_64 viewer with no `SDL3.dll`, libgcc, libstdc++, or winpthread
imports; Windows system DLLs remain normal operating-system dependencies. The
macOS archive contains a Mach-O universal viewer with x86_64 and arm64 slices,
and its static SDL3 linkage was verified by the native macOS CI jobs.

The release archives are unsigned and the macOS bundle is not notarized.
Display-backed GUI evidence and native Windows runtime evidence remain
deferred as recorded above.
