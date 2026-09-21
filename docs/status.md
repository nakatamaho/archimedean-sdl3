# Status

Current milestone: M1 — complete.

## Dependency baseline

- SDL target release: 3.4.16
- nlohmann/json target release: 3.12.0
- SageMath version: to be recorded during M2
- PyNormaliz/Normaliz version: to be recorded during M2

## Completed work

- M0 — repository bootstrap and documentation scaffold committed.
- M1 — vendored dependency and CMake/headless executable baseline committed.

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
```
```
