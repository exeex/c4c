# Curated EASTL C++ External Subset

This directory is the bootstrap location for curated EASTL-derived C++ tests
that run inside c4c's own test harness.

Current intent:

- keep `ref/EASTL` and `ref/EABase` as the upstream header/library sources
- keep `tests/cpp/external/eastl` focused on curated standalone cases and their
  c4c runner metadata
- grow coverage incrementally from one proven case instead of importing the
  full upstream `ref/EASTL/test` executable harness in one step

Layout:

- `allowlist.txt`: manifests the registered cases and their execution mode
- `RunCase.cmake`: runner that injects shared EASTL/EABase include paths
- `UPSTREAM.md`: provenance notes for extracted cases
- `type_traits/`: first standalone case family

Allowlist format:

- one entry per line
- blank lines and `#` comments are ignored
- syntax: `relative/path.cpp|parse`
- syntax: `relative/path.cpp|frontend`
- syntax: `relative/path.cpp|runtime`

Runner semantics:

- `parse`: `c4cll --parse-only` must succeed with EASTL include paths
- `frontend`: `c4cll` must successfully emit LLVM IR for the case
- `runtime`: host compile+run and c4c compile+run must both succeed
