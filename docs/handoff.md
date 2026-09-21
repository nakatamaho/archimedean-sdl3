# Handoff

Project complete through M11. The v1.2 regular-solid extension remains the
published baseline, and v1.3 adds X11-ico-style tumbling motion plus an
SDL3-window help overlay.

No implementation milestone remains. External evidence still deferred:
display-backed Linux/Windows visual run and native Windows runtime. Do not
claim either from offscreen execution, Wine, cross-compilation, or CI.

The MinGW configuration statically links vendored SDL3 with
`SDL_SHARED=OFF` and `SDL_STATIC=ON`, and uses `-static -static-libgcc
-static-libstdc++` for executables. No SDL3, GCC, or C++ runtime DLL is
expected beside the viewer; Windows system DLLs remain normal dependencies.

The canonical `data/archimedean.json` is embedded into the viewer at build
time. It contains the five Platonic solids followed by the 13 Archimedean
solids. The default executable no longer requires a JSON file or a particular
working directory; `--data PATH` explicitly selects an external override.

The project version is `1.3.0` on `main`. The published release remains
`v1.2.0`; no v1.3 archive was requested. The release workflow builds native Intel and
Apple Silicon macOS bundles, a Linux x86_64 archive with static SDL3, and a
fully static MinGW x86_64 archive. It combines the macOS architectures with
`lipo` and publishes all platform archives on the `v1.2.0` release. The
archives are unsigned and notarization remains deferred.

M0 through M11 evidence is recorded in `docs/status.md`. The public remote is
`https://github.com/nakatamaho/archimedean-sdl3`, with `main` as its default
branch.
