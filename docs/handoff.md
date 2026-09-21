# Handoff

Project complete through M10.

No implementation milestone remains. External evidence still deferred:
display-backed Linux/Windows visual run and native Windows runtime. Do not
claim either from offscreen execution, Wine, cross-compilation, or CI.

The MinGW configuration statically links vendored SDL3 with
`SDL_SHARED=OFF` and `SDL_STATIC=ON`, and uses `-static -static-libgcc
-static-libstdc++` for executables. No SDL3, GCC, or C++ runtime DLL is
expected beside the viewer; Windows system DLLs remain normal dependencies.

M0 through M10 evidence is recorded in `docs/status.md`. The public remote is
`https://github.com/nakatamaho/archimedean-sdl3`, with `main` as its default
branch.
