# Status

Current milestone: M0 — complete.

## Dependency baseline

- SDL target release: 3.4.16
- nlohmann/json target release: 3.12.0
- SageMath version: to be recorded during M2
- PyNormaliz/Normaliz version: to be recorded during M2

## Completed work

- M0 — repository bootstrap and documentation scaffold committed.

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

