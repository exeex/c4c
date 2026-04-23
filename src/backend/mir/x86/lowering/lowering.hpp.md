# `lowering/lowering.hpp` contract

## Legacy Evidence

The legacy x86 route mixed encoding helpers, policy decisions, and module
control flow inside larger codegen files. This seam exists so semantic-family
helpers stay public without letting x86 lowering reclaim prealloc ownership.

## Design Contract

This header is the single public header for `x86/lowering/`.

Owned inputs:

- backend-owned BIR type information needed for size-name selection
- already-published frame and call facts expressed as byte offsets or summary
  requests
- function names used only for emitted comment text

Owned outputs:

- narrow summary strings for stable lowering families
- small comment and memory-formatting helpers used by x86 emission

Invariants:

- lowering helpers are encoding helpers only; they must not derive new
  regalloc, frame, call-placement, or dynamic-stack policy
- returned summary strings describe the live contract family and are allowed to
  stay compact because detailed behavior belongs in implementation or sibling
  contracts
- `stack_mem(...)` formats a fixed byte-offset reference only; it must not
  guess from transient `rsp` state when dynamic stack motion exists
- memory-size naming is driven by existing BIR type kinds, not by target-local
  reinterpretation of backend semantics

Direct dependencies:

- `backend/bir/bir.hpp` for type-kind queries used by `memory_size(...)`
- published prepared frame or call facts supplied by callers in `module/` or
  other owned x86 consumers

Deferred behavior or upstream gaps:

- canonical frame layout, VLA handling, and dynamic-stack planning remain
  upstream in `prealloc/`
- whole-module routing, prepared-query ownership, and debug observation belong
  to `module/`, `prepared/`, and `debug/`

Must not own:

- whole-module routing
- prepared-query archaeology
- public backend entry routing
- target admission or compatibility shims
