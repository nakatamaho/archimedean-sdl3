# Milestones

Complete milestones in order. Do not start a second milestone in the same `/goal` run.

| Milestone | Goal | Acceptance | Difficulty |
|---|---|---|---|
| M0 | Create public GitHub repository and documentation scaffold | Remote exists, main branch exists, BSD-2-Clause license present, docs committed, clean worktree | Low |
| M1 | Pin dependencies and establish CMake/headless executable skeleton | SDL 3.4.16 and nlohmann/json 3.12.0 pinned as submodules; Linux and MinGW configuration paths exist; `--help` and `--selftest` skeleton build | Medium |
| M2 | Implement SageMath generator for all 13 solids | `sage -python tools/generate_archimedean.py` produces JSON; all combinatorial/topological checks pass; repeated generation is deterministic | High |
| M3 | Add independent CPython JSON validator and commit canonical data | `data/archimedean.json` committed; validator passes without Sage; corrupt-data negative tests exist | Medium |
| M4 | Implement C++ model loader and math library | All 13 solids load; topology validated; vector/matrix/axis-angle/projection unit tests pass headlessly | Medium |
| M5 | First SDL3 filled rotating solid | Linux interactive window renders one selected solid with filled faces, projection, back-face culling, face depth ordering | Medium |
| M6 | Flat Lambert shading and polygon palette | Face normals correct; face colors vary by polygon size and illumination; lighting toggle works | Medium |
| M7 | Runtime controls | solid cycling, arbitrary rotation axis, speed, pause, reverse, reset, zoom, wireframe toggle and title status all work | Medium |
| M8 | Windows/MinGW support | Native MinGW-w64 build succeeds; DLL handling documented; native Windows `--selftest` PASS; GUI runtime evidence recorded if available | Medium |
| M9 | GitHub Actions CI | Ubuntu and Windows/MinGW jobs build and run validator, CTest, and `--selftest`; no false visual PASS claims | Medium |
| M10 | Release-quality QA and documentation | clean rebuilds, generator provenance, README controls/build docs, screenshots from real runs, release checklist complete | Medium |

## M0 — Repository bootstrap

### Work

- Run `gh auth status`.
- Create or clone `archimedean-sdl3` without touching unrelated repositories.
- Add `LICENSE`, `.gitignore`, `README.md`, `AGENTS.md`, `CODEX.md`, and `docs/*`.
- Commit only documentation/scaffolding.

### Tests/evidence

```sh
git status --short
git remote -v
git log -1 --oneline
gh repo view --json nameWithOwner,url,visibility,defaultBranchRef
```

### Gate

Stop if GitHub authentication is unavailable.

## M1 — Dependency/CMake baseline

### Work

- Add SDL and nlohmann/json submodules at pinned releases.
- Create CMake targets and minimal main/test executables.
- Add system-dependency option.
- Add MinGW toolchain file.

### Tests

Linux configure/build + CTest. If Windows/MinGW is not available yet, compile-path evidence may be deferred to M8 and must not be called PASS.

## M2 — SageMath generator

### Work

- Implement exactly 13 constructors.
- Normalize, cyclically order faces, enforce outward winding, canonicalize numbering/order.
- Validate V/E/F, histogram, manifold edges, Euler relation, planarity, edge equality, finiteness.
- Record SageMath version in JSON.

### Special case

Use the Sage-supported Normaliz route for snub dodecahedron. If PyNormaliz is absent, report the exact install blocker instead of substituting hand-entered coordinates.

### Tests

- Generate twice to two temporary paths.
- Compare canonical solid payloads byte-for-byte after excluding only intentionally varying metadata; preferably make the complete output byte-identical.
- Validate known counts for every solid.

## M3 — Canonical JSON and independent validator

### Work

- Implement pure CPython validator with no NumPy/Sage requirement.
- Commit the generator output.
- Add negative fixtures/tests for bad index, duplicate/missing manifold edge, wrong count, and NaN-like invalid input if parser permits.

### Tests

```sh
python3 tools/validate_archimedean.py data/archimedean.json
```

## M4 — Loader and math

### Work

- Define `Vec3`, `Mat3` or minimal quaternion/axis-angle helpers.
- Implement robust normalization and finite checks.
- Load schema v1 using nlohmann/json.
- Validate runtime model invariants.
- Implement perspective projection independently of SDL.

### Tests

Unit tests cover math, all data files/solids, invalid schema, invalid indices, and representative projection points.

## M5 — SDL3 rotating renderer

### Work

- Create resizable SDL3 window and renderer.
- Render default truncated icosahedron.
- Integrate rotation from elapsed monotonic time.
- Back-face cull.
- Fan-triangulate visible convex polygons at render time.
- Sort visible faces back-to-front.
- Submit through `SDL_RenderGeometry()`.

### Gate

Interactive visual PASS requires a real display. If working over SSH/headless CI, record build/selftest only and defer visual evidence.

## M6 — Shading

### Work

- Compute view-space flat normals.
- Add fixed directional light and ambient+diffuse Lambert intensity.
- Add fixed base-color palette keyed by polygon side count.
- Add lighting toggle.

### Visual acceptance

Faces oriented toward the light are visibly brighter; silhouettes and face boundaries remain stable while rotating.

## M7 — Controls

Implement every control from `CODEX.md` and update the window title. Axis edits must remain normalized and stable.

Test keyboard state transitions separately from the SDL window where practical.

## M8 — Windows/MinGW

### Work

- Build in native Windows MSYS2 MinGW-w64 environment.
- Verify the executable finds SDL3.dll.
- Run CTest and `--selftest` natively.
- Launch GUI and exercise controls when a GUI session is available.

Do not count a Linux-to-MinGW cross-compile as native runtime evidence.

## M9 — CI

Add Ubuntu and Windows/MSYS2 MinGW jobs. Pin action major versions. Checkout submodules recursively. CI must not require Sage regeneration in ordinary pushes.

## M10 — Release QA

- Fresh clone with recursive submodules.
- Linux Release build + tests + real GUI screenshot.
- Windows MinGW Release build + tests + real GUI screenshot when available.
- Regenerate JSON from documented SageMath environment and compare.
- Update README and handoff.
- Tag only after all non-deferred release gates pass.

