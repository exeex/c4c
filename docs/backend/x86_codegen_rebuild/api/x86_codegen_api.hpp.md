# `api/x86_codegen_api.hpp`

Primary role: public front-door header for x86 codegen entrypoints and
assembly requests.

Owned inputs:

- direct BIR modules that still need target resolution before preparation
- prepared modules that are already validated enough for x86 module emission
- LIR modules when callers request the direct LIR entry surface
- high-level assembly/emit options that belong at the public API boundary

Owned outputs:

- stable declarations for `emit_module(...)`, `emit_prepared_module(...)`, and
  `assemble_module(...)`-style entrypoints
- API-visible result/status types that callers need before they enter the
  internal module and lowering seams

Allowed indirect queries:

- `core/x86_codegen_types.hpp` for shared public-facing data carriers
- `module/module_emit.hpp` for the actual module-orchestration entrypoint
- `abi/x86_target_abi.hpp` only for target-profile typing that must appear at
  the public boundary

Forbidden knowledge:

- stack layout, value-home, addressing, or call-lane policy
- prepared fast-path matcher details
- route-debug formatting or debug-only trace helpers
- module-data section rendering details

Role classification:

- `api`

Drafting notes:

- keep this header narrow; it replaces the public subset of
  `x86_codegen.hpp` without reintroducing mixed helper declarations
- declarations here may forward users into preparation or module emission, but
  they must not publish the full internal dispatch context
