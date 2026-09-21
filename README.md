# archimedean-sdl3

`archimedean-sdl3` is a small, portable viewer for 18 regular solids: the five
Platonic solids and all 13 Archimedean solids. SageMath is used only to
generate the canonical geometry. The committed JSON preserves polygon faces,
while the C++17 SDL3 viewer performs CPU-side transforms, culling, Lambert
shading, depth ordering, and render-time fan triangulation.

## Screenshot

> Screenshot placeholder: a real interactive screenshot will be added after a
> display-backed Linux or Windows run. Headless/offscreen execution is not
> represented as visual evidence.

## Supported solids

The canonical artifact contains these IDs, in deterministic order:

| ID | Solid |
| --- | --- |
| `tetrahedron` | Tetrahedron |
| `cube` | Cube (hexahedron) |
| `octahedron` | Octahedron |
| `dodecahedron` | Dodecahedron |
| `icosahedron` | Icosahedron |
| `truncated_tetrahedron` | Truncated tetrahedron |
| `cuboctahedron` | Cuboctahedron |
| `truncated_cube` | Truncated cube |
| `truncated_octahedron` | Truncated octahedron |
| `rhombicuboctahedron` | Small rhombicuboctahedron |
| `truncated_cuboctahedron` | Great rhombicuboctahedron |
| `snub_cube` | Snub cube |
| `icosidodecahedron` | Icosidodecahedron |
| `truncated_dodecahedron` | Truncated dodecahedron |
| `truncated_icosahedron` | Truncated icosahedron |
| `rhombicosidodecahedron` | Small rhombicosidodecahedron |
| `truncated_icosidodecahedron` | Great rhombicosidodecahedron |
| `snub_dodecahedron` | Snub dodecahedron |

The two snub solids are generated through Sage's supported chiral constructors;
the snub dodecahedron uses Sage's Normaliz backend.

## Linux build and tests

The default build uses the pinned SDL3 and nlohmann/json submodules:

```sh
git clone --recursive https://github.com/nakatamaho/archimedean-sdl3.git
cd archimedean-sdl3
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
python3 tools/validate_archimedean.py data/archimedean.json
./build/archimedean_viewer --selftest
```

`--selftest` does not create a window. The normal viewer starts with the
truncated icosahedron, ico-style motion, 30 degrees/second, and a 1000 x 800
resizable window. Its custom-axis mode starts with a normalized Y axis. The
canonical JSON is embedded into the executable at build time, so the default
launch does not depend on the current working directory or a separate JSON
file. Use `--data PATH` to override it with an external JSON file:

```sh
./build/archimedean_viewer --data data/archimedean.json
```

Since v1.3, the default animation follows the classic X11 `ico` style: each
elapsed-time update composes equal X- and Y-axis rotations, producing a
continuous tumbling motion instead of spinning around one fixed axis. Press
`I` to switch between this motion and the editable arbitrary-axis mode. The
axis keys and arrow keys automatically select arbitrary-axis mode.

An optional `ARCHVIEW_USE_SYSTEM_DEPS=ON` CMake mode is available for packagers
with SDL3 and nlohmann/json config packages installed.

## Windows and MinGW-w64

For a native MSYS2 UCRT64 shell, install GCC, CMake, Ninja, and Python, then
run the same configure/build/test commands using `cmake` and `ctest` from that
environment. The supported CMake cross-toolchain path is:

```sh
cmake -S . -B build-mingw -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake \
  -DSDL_SHARED=OFF -DSDL_STATIC=ON
cmake --build build-mingw --parallel
```

The MinGW configuration statically links SDL3 and the GCC/C++ runtime, and
embeds the canonical JSON. The viewer does not require adjacent `SDL3.dll`,
`libgcc_s_seh-1.dll`, `libstdc++-6.dll`, or JSON files. Windows system DLLs
such as `KERNEL32.dll` remain normal operating-system dependencies. A MinGW
cross-compile is not native Windows runtime evidence. See
[`docs/status.md`](docs/status.md) for the current platform evidence and
limitations.

## macOS

The macOS build uses a native SDL3 static library and produces an application
bundle. On an Intel or Apple Silicon runner:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON \
  -DSDL_SHARED=OFF -DSDL_STATIC=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
open build/archimedean_viewer.app
```

The `v1.2.0` release provides these archives:

- [macOS universal](https://github.com/nakatamaho/archimedean-sdl3/releases/download/v1.2.0/archimedean-sdl3-v1.2.0-macos-universal.tar.gz), with SDL3 statically linked;
- [Linux x86_64](https://github.com/nakatamaho/archimedean-sdl3/releases/download/v1.2.0/archimedean-sdl3-v1.2.0-linux-x86_64.tar.gz), with SDL3 statically linked (system graphics/audio libraries remain operating-system dependencies);
- [MinGW x86_64](https://github.com/nakatamaho/archimedean-sdl3/releases/download/v1.2.0/archimedean-sdl3-v1.2.0-mingw-x86_64.tar.gz), fully static for SDL3 and the GCC/C++ runtime (Windows system DLLs remain operating-system dependencies).

The archives are unsigned and the macOS bundle is not notarized; macOS may
require the user to approve it in Privacy & Security before opening it. The
release checksums are in [`SHA256SUMS`](https://github.com/nakatamaho/archimedean-sdl3/releases/download/v1.2.0/SHA256SUMS).

## SageMath generation

The generator requires SageMath 10.9 or a compatible Sage release, with
PyNormaliz/Normaliz available for the snub dodecahedron route:

```sh
sage -python tools/generate_archimedean.py --output data/archimedean.json
python3 tools/validate_archimedean.py data/archimedean.json
```

The tested Conda-forge environment used by this repository can be invoked as:

```sh
micromamba run -p /path/to/sage-env python \
  tools/generate_archimedean.py --output data/archimedean.json
```

The ordinary viewer does not import or require SageMath. Repeating generation
in the same environment must produce byte-identical JSON; compare temporary
outputs with `cmp -s` before replacing the committed artifact.

## Controls

| Key | Action |
| --- | --- |
| Esc | Quit |
| Space | Pause/resume rotation |
| H | Toggle the in-window help overlay |
| I | Toggle X11 ico-style motion / custom-axis motion |
| N / PageDown | Next solid |
| P / PageUp | Previous solid |
| 1 / 2 / 3 | Set rotation axis to X / Y / Z |
| Left / Right | Decrease/increase axis azimuth by 5 degrees |
| Up / Down | Increase/decrease axis elevation by 5 degrees |
| `[` / `]` | Decrease/increase angular speed |
| Backspace | Reverse rotation direction |
| R | Reset object orientation |
| W | Toggle wireframe overlay |
| L | Toggle Lambert lighting |
| `+` / `-` | Zoom in/out |
| Home | Restore default view, axis, speed, and zoom |

Speed is clamped to `[-360, 360]` degrees/second and the axis is normalized
after every edit. The title reports the selected solid, speed, axis, motion
mode, and paused/running state. The H overlay repeats the complete keyboard
control list inside the SDL3 window.

## Data and rendering

Schema version 1 contains generator metadata and exactly 18 solids. Each solid
has finite normalized vertices, explicit polygon `faces`, explicit combinatorial
`edges`, and `statistics` containing V/E/F and the face-size histogram. The
CPython validator and C++ loader independently check counts, Euler
characteristic, manifold edge incidence, planarity, outward winding, and edge
normalization. Faces remain polygons in JSON; only the renderer creates
transient triangle fans.

The viewer uses a right-handed CPU coordinate system with positive view-space
Z forward, back-face culling, far-to-near visible-face sorting, flat
ambient-plus-diffuse Lambert shading, and `SDL_RenderGeometry()`.

## License and dependencies

Project-authored code is licensed under the [BSD-2-Clause](LICENSE) license.
SDL 3.4.16 and nlohmann/json 3.12.0 are pinned as git submodules under
`external/`. SageMath, PyNormaliz, and Normaliz are generator-time
dependencies only. See [`docs/spec.md`](docs/spec.md),
[`docs/milestones.md`](docs/milestones.md), and
[`docs/release_checklist.md`](docs/release_checklist.md) for implementation and
release evidence.
