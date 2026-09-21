# Codex execution guide — Archimedean SDL3 Viewer

## 1. Project goal

Create a public GitHub repository named `archimedean-sdl3` containing:

1. a SageMath-based Python generator for the 13 Archimedean solids;
2. deterministic polygon data in `data/archimedean.json`;
3. a portable C++17 SDL3 viewer for Linux and Windows/MinGW-w64;
4. filled polygon rendering with flat Lambert shading;
5. interactive control of the rotation axis and angular speed;
6. CI that builds and runs non-GUI self-tests on Linux and MinGW/Windows.

The polygon data is the canonical artifact. Preserve the original polygon faces. Do not permanently triangulate the JSON. Triangulate convex faces only in the viewer immediately before rendering.

## 2. Non-goals for v1

Do not add OpenGL, Vulkan, Direct3D, SDL_gpu, Dear ImGui, SDL_ttf, physics, texture mapping, mesh editing, arbitrary OBJ/glTF import, or macOS support in v1. Do not introduce a custom GPU shader language in v1.

The initial renderer must use SDL3's ordinary renderer API and `SDL_RenderGeometry()` after CPU-side 3D transformation, culling, lighting, projection, and depth sorting.

## 3. Repository creation

Repository name: `archimedean-sdl3`

License for project-authored code: BSD-2-Clause.

If GitHub CLI is authenticated and the repository does not already exist:

```sh
gh auth status
gh repo create archimedean-sdl3 --public --clone --license bsd-2-clause --description "SageMath-generated Archimedean solids with an SDL3 interactive shaded viewer"
cd archimedean-sdl3
```

If the repository already exists, clone it instead. Never delete or replace an existing remote repository.

If `gh auth status` fails, stop M0 and report the exact authentication blocker. Do not invent success.

The first repository commit must contain documentation/scaffolding only. Implementation starts in M1.

## 4. Third-party dependencies

Use git submodules for reproducible native builds.

- SDL: pin to SDL 3.4.16 initially, under `external/SDL`.
- nlohmann/json: pin to v3.12.0 initially, under `external/json`.

Record the exact submodule commit IDs in `docs/status.md`.

CMake must support two modes:

- vendored/submodule dependencies: default;
- system SDL3 and system nlohmann/json: optional CMake switch for packagers.

Never copy third-party source files into project-authored source directories.

## 5. Required repository layout

```text
archimedean-sdl3/
├── .github/
│   └── workflows/
│       └── ci.yml
├── external/
│   ├── SDL/                  # git submodule
│   └── json/                 # git submodule
├── cmake/
│   └── mingw-w64-x86_64.cmake
├── data/
│   └── archimedean.json
├── docs/
│   ├── spec.md
│   ├── milestones.md
│   ├── status.md
│   └── handoff.md
├── include/
│   └── archview/
│       ├── math3d.hpp
│       ├── polyhedron.hpp
│       └── renderer.hpp
├── src/
│   ├── main.cpp
│   ├── math3d.cpp
│   ├── polyhedron.cpp
│   └── renderer.cpp
├── tests/
│   ├── test_json.cpp
│   ├── test_math.cpp
│   └── test_topology.cpp
├── tools/
│   ├── generate_archimedean.py
│   └── validate_archimedean.py
├── AGENTS.md
├── CODEX.md
├── CMakeLists.txt
├── LICENSE
└── README.md
```

Do not create extra architecture layers without a demonstrated need.

## 6. Language and coding rules

- C++17.
- Python 3 syntax, executed through SageMath where required.
- All source-code identifiers, comments, diagnostics, filenames, and documentation intended for developers must be English.
- No `using namespace std;` in headers.
- Avoid exceptions crossing the SDL event/render loop boundary. Convert loader failures into clear startup diagnostics.
- Keep math code independent of SDL so it can be unit tested headlessly.
- No platform-specific rendering code in project sources for v1.
- Treat compiler warnings as useful evidence; CI must use `-Wall -Wextra -Wpedantic` on GCC/MinGW where applicable.

## 7. Data generator contract

`tools/generate_archimedean.py` must be executable as:

```sh
sage -python tools/generate_archimedean.py --output data/archimedean.json
```

It must generate exactly these 13 solids:

1. truncated tetrahedron
2. cuboctahedron
3. truncated cube
4. truncated octahedron
5. rhombicuboctahedron
6. truncated cuboctahedron
7. snub cube
8. icosidodecahedron
9. truncated dodecahedron
10. truncated icosahedron
11. rhombicosidodecahedron
12. truncated icosidodecahedron
13. snub dodecahedron

Use SageMath `polytopes` constructors. The snub dodecahedron must use `base_ring=AA` and the Normaliz backend when required by the installed SageMath version.

Do not copy coordinate tables from third-party websites or repositories.

### 7.1 Normalization

For every solid:

- translate the vertex centroid to `(0,0,0)`;
- scale the geometry so the mean edge length is `1.0`;
- preserve polygon faces;
- order every face counter-clockwise when viewed from outside;
- assign deterministic vertex numbering;
- assign deterministic face ordering.

The output must be stable across repeated generation with the same SageMath version, aside from the recorded generator metadata.

### 7.2 JSON schema

Top-level structure:

```json
{
  "schema_version": 1,
  "generator": {
    "program": "tools/generate_archimedean.py",
    "sage_version": "..."
  },
  "solids": [
    {
      "id": "truncated_icosahedron",
      "display_name": "Truncated icosahedron",
      "vertices": [[0.0, 0.0, 0.0]],
      "faces": [[0, 1, 2]],
      "edges": [[0, 1]],
      "statistics": {
        "vertices": 60,
        "edges": 90,
        "faces": 32,
        "face_histogram": {"5": 12, "6": 20}
      }
    }
  ]
}
```

Numbers in `vertices` are JSON floating-point numbers. Use sufficient digits for round-trip loading into IEEE binary64. Do not serialize NaN or infinity.

### 7.3 Required generator validation

Generation must fail nonzero if any check fails:

- exactly 13 solids;
- expected V/E/F counts;
- expected polygon-size histogram;
- Euler characteristic `V - E + F == 2`;
- every edge belongs to exactly two faces;
- every face has at least three distinct vertices;
- indices are in range;
- no duplicate edge inside one face;
- face planarity within a documented tolerance;
- normalized edge lengths agree within a documented tolerance;
- outward face winding;
- finite coordinates only.

Keep the known expected combinatorics in one explicit table inside the generator or validator so errors are easy to audit.

`tools/validate_archimedean.py` must be ordinary CPython and must validate the committed JSON without requiring SageMath. This script is the validator used by normal CI.

## 8. Viewer contract

Executable name: `archimedean_viewer`.

Minimum invocation:

```sh
./archimedean_viewer --data data/archimedean.json
```

Supported startup options:

```text
--data PATH
--solid ID
--speed DEG_PER_SEC
--axis X,Y,Z
--width PIXELS
--height PIXELS
--selftest
--help
```

Default solid: `truncated_icosahedron`.
Default rotation axis: normalized `(0,1,0)`.
Default angular speed: `30 deg/s`.
Default window: `1000 x 800`, resizable.

`--selftest` must not create a window. It must load the JSON, verify all 13 solids, exercise representative math/projection paths, and return 0 only on success.

## 9. Renderer design

The renderer is deliberately simple and portable.

Per frame:

1. integrate rotation using elapsed monotonic time, not frame count;
2. rotate object-space vertices using a normalized arbitrary axis;
3. transform into camera/view space;
4. compute each polygon face normal in view space;
5. back-face cull polygons facing away from the camera;
6. calculate flat Lambert shading per face;
7. triangulate each convex polygon as a triangle fan for submission only;
8. calculate a face depth key and sort visible faces back-to-front;
9. perspective-project to screen coordinates;
10. submit triangles with `SDL_RenderGeometry()`;
11. optionally draw polygon edges as a wireframe overlay.

Do not modify the JSON polygon topology to triangles.

### 9.1 Camera

Use a right-handed internal 3D coordinate system and document the sign convention.

Use perspective projection with:

- configurable or constant vertical FOV around 45 degrees;
- object centered at the origin;
- camera far enough away that every supported solid fits initially;
- resize-aware aspect ratio;
- near-plane rejection sufficient to avoid division by zero.

The exact convention matters less than consistency and tests.

### 9.2 Lighting and colors

Use flat face shading, because these are polyhedra.

A face base color is selected by polygon size so triangles, squares, pentagons, hexagons, octagons, and decagons are visually distinct. Use a fixed palette defined in one source location.

Lighting:

```text
intensity = ambient + diffuse * max(0, dot(normal, light_direction))
```

Suggested initial values:

```text
ambient = 0.25
diffuse = 0.75
```

Clamp final RGB values. Lighting must not change alpha.

Do not add specular lighting in v1.

## 10. Interactive controls

Required controls:

```text
Esc                 Quit
Space               Pause/resume rotation
N / PageDown        Next solid
P / PageUp          Previous solid
1                   Rotation axis = X
2                   Rotation axis = Y
3                   Rotation axis = Z
Left / Right        Rotate axis azimuth by -/+ 5 degrees
Up / Down           Rotate axis elevation by +/- 5 degrees
[ / ]               Decrease/increase angular speed
Backspace           Reverse rotation direction
R                   Reset object orientation
W                   Toggle wireframe overlay
L                   Toggle Lambert lighting
+ / -               Zoom in/out
Home                Restore default view, axis, speed, and zoom
```

Clamp angular speed to a documented safe range, for example `[-360, +360] deg/s`.

Normalize the axis after every edit. Avoid singular behavior at the poles by clamping elevation or using a vector/quaternion edit representation.

Update the SDL window title at a modest rate, not every event, to show:

```text
<solid> | speed=<...> deg/s | axis=(x,y,z) | paused/running
```

No font dependency is required in v1.

## 11. CMake requirements

Use modern target-based CMake.

Required targets:

```text
archview_core          # math, JSON model, validation; no SDL dependency
archview_renderer      # SDL-facing renderer
archimedean_viewer     # executable
archview_tests         # headless tests
```

`archview_core` must be testable without opening a window.

Default vendored mode:

```cmake
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)
add_subdirectory(external/json EXCLUDE_FROM_ALL)
```

Link with the stable SDL target:

```cmake
target_link_libraries(archimedean_viewer PRIVATE SDL3::SDL3)
```

Provide an option such as:

```text
ARCHVIEW_USE_SYSTEM_DEPS=OFF
```

When enabled, use `find_package(SDL3 CONFIG REQUIRED)` and `find_package(nlohmann_json CONFIG REQUIRED)`.

On Windows shared-SDL builds, ensure `SDL3.dll` is copied next to the executable when needed.

## 12. Platform requirements

### Linux

Supported development compiler: GCC or Clang on a maintained x86_64 Linux distribution.

Acceptance requires:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/archimedean_viewer --selftest --data data/archimedean.json
```

A real interactive run must also be documented when a display is available. Headless CI is not evidence that an SDL window rendered correctly.

### Windows / MinGW-w64

Primary supported Windows compiler: MinGW-w64 x86_64.

Support both:

1. native MSYS2 MinGW64/UCRT64 development;
2. a CMake MinGW toolchain file where useful.

Do not mark the Windows GUI path as runtime-tested unless it was actually launched on Windows.

## 13. CI

Create `.github/workflows/ci.yml` with at least:

- Ubuntu + GCC/Ninja;
- Windows + MSYS2 MinGW-w64/Ninja.

Each job must:

1. checkout submodules recursively;
2. configure;
3. build;
4. run CTest;
5. run `tools/validate_archimedean.py data/archimedean.json`;
6. run viewer `--selftest` without creating a window.

Do not attempt to prove visual correctness from a headless runner.

SageMath regeneration is not mandatory on every ordinary CI run. The generated JSON is committed and validated independently. Add a separate manual regeneration/verification workflow only after the basic project is stable.

## 14. Documentation requirements

`README.md` must include:

- screenshot placeholder until real screenshots exist;
- project purpose;
- supported solids;
- Linux build instructions;
- Windows/MinGW build instructions;
- SageMath generation instructions;
- controls table;
- data schema summary;
- license and third-party dependency note.

Never claim a screenshot, platform run, or test happened unless there is evidence from the current repository state.

`docs/status.md` records completed milestones and exact commands/results.

`docs/handoff.md` contains only the current next work, blockers, and important implementation facts. Keep it short.

## 15. Git discipline

- One milestone per commit or small milestone commit series.
- Conventional commit subjects are preferred.
- Never rewrite published history unless explicitly instructed by the user.
- Never use `git reset --hard` on a dirty worktree.
- Never use plain `git push --force`.
- Do not commit build trees, generated executables, DLLs, IDE state, or private files.
- `data/archimedean.json` is intentionally committed.
- Do not advance to a later milestone while the current milestone acceptance criteria fail.

## 16. Milestone execution rule

When invoked by a `/goal` prompt, Codex must complete exactly one next eligible unfinished milestone from `docs/milestones.md` unless the prompt explicitly says otherwise.

Before implementation:

1. read `AGENTS.md`, `CODEX.md`, `docs/spec.md`, `docs/milestones.md`, `docs/status.md`, and `docs/handoff.md`;
2. inspect actual repository state;
3. identify the first unfinished dependency-satisfied milestone;
4. run its preflight checks;
5. implement only that milestone and its named tests;
6. record exact commands and results;
7. update `docs/status.md` and `docs/handoff.md`;
8. stop.

If a required external dependency, authentication, display, or platform is unavailable, record the blocker. Never relabel unavailable evidence as PASS.

