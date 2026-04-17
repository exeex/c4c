# Upstream Provenance

This directory contains standalone cases derived from upstream EASTL test
coverage.

The upstream source-of-truth library and larger test harness remain under:

- `ref/EASTL`
- `ref/EABase`

Current extracted case mapping:

- `piecewise_construct/frontend_basic.cpp`
  - upstream reference: `ref/EASTL/include/EASTL/internal/piecewise_construct_t.h`
  - extraction style: reduced standalone frontend smoke test
  - reason: bootstrap the external EASTL runner with a case that proves c4c
    can emit LLVM IR for a minimal EASTL-owned API surface without importing
    the full `EASTLTest` executable harness

Follow-on work may replace this bootstrap directory with a dedicated submodule,
but the case paths and allowlist/runner contract should stay stable.
