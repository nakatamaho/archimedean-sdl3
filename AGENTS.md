# AGENTS.md

## Scope

These rules apply to the entire repository.

## Primary objective

Build a small, auditable Archimedean-solid data generator and SDL3 viewer. Favor correctness, deterministic polygon topology, portability, and testability over framework complexity.

## Required reading order

Before modifying the repository, read:

1. `CODEX.md`
2. `docs/spec.md`
3. `docs/milestones.md`
4. `docs/status.md`
5. `docs/handoff.md`

## Hard constraints

- Preserve polygon faces in JSON; triangulation is render-time only.
- SageMath generates the canonical geometry data.
- The ordinary viewer must not require SageMath.
- Linux and Windows/MinGW-w64 are release platforms for v1.
- Use SDL3, C++17, CMake, and nlohmann/json.
- Renderer v1 uses CPU-side 3D math plus `SDL_RenderGeometry()`.
- Keep math/model code independent from SDL.
- All code and code comments must be English.
- Do not add unrelated features while completing a milestone.

## Evidence

A build command is not a runtime test. A headless self-test is not a visual test. A cross-compile is not a native Windows runtime test. Record these distinctions precisely.

