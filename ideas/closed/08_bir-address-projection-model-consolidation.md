# BIR Address Projection Model Consolidation

## Intent

Consolidate BIR memory address/projection reasoning so local and global GEP
families use the same shared projection vocabulary where their semantics match.

This should happen after local GEP has its own implementation file. The goal is
not to create a new state owner or a large header surface; the goal is to make
the existing `memory_helpers.hpp` projection facts more consistently useful
across `addressing.cpp`, `local_gep.cpp`, and related memory code.

## Rationale

The current memory implementation already has shared helpers such as:

```text
resolve_aggregate_byte_offset_projection
resolve_scalar_layout_facts_at_byte_offset
can_reinterpret_byte_storage_as_type
```

But address/projection policy still lives in several places:

- global and relative GEP handling in `memory/addressing.cpp`
- local aggregate/pointer/array GEP handling in the future
  `memory/local_gep.cpp`
- scalar subobject checks in provenance paths
- byte-storage reinterpretation and repeated extent handling across memory
  files

This initiative should reduce duplicated layout/projection reasoning without
changing lowering semantics.

## Constraints

- Do not add new `.hpp` files unless the active plan explicitly proves the
  existing two memory headers cannot carry the vocabulary clearly.
- Prefer using existing:
  - `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
  - `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- Keep `BirFunctionLowerer` as the owner of stateful lowering operations.
- Do not convert stateful lowering to free functions taking
  `BirFunctionLowerer& self`.
- Do not change BIR output, diagnostics, or testcase expectations.

## Refactoring Steps

1. Inventory projection paths.
   - Compare local GEP projection logic, global GEP projection logic, and
     provenance scalar subobject logic.
   - Identify repeated layout walks and repeated offset calculations.
   - Keep caller-specific policy at the caller.

2. Strengthen shared helper result types only where needed.
   - Prefer small additions to existing helper result structs over new headers.
   - Keep helpers pure and argument-driven.
   - Avoid helpers that mutate lowerer state.

3. Reuse projection helpers consistently.
   - Make local and global GEP code call the same projection helper for
     equivalent aggregate traversal.
   - Keep local/global-specific decisions in their implementation files.

4. Normalize naming.
   - Use consistent terms for byte offset, child type, child index, element
     stride, repeated extent, and scalar leaf facts.
   - Avoid adding near-duplicate helper names for the same fact.

5. Build and prove no behavior change.
   - Build `c4c_codegen`.
   - Run relevant BIR/LIR-to-BIR GEP, aggregate, provenance, and memory tests.
   - Do not rewrite expectations.

## Acceptance Criteria

- Equivalent local/global aggregate projection logic uses shared helper facts.
- `memory_helpers.hpp` remains a normal pure helper declaration surface.
- No new `.hpp` files are added unless explicitly justified by the active plan.
- Stateful lowering remains in `BirFunctionLowerer` members.
- Caller-specific local/global policy remains readable at the call sites.
- `c4c_codegen` builds.
- Relevant GEP/provenance/memory tests pass with no expectation rewrites.

## Closure Note

Closed after the active runbook completed all five steps. The implementation
strengthened shared helper facts, reused the shared projection helpers across
local and global GEP paths, normalized projection offset naming, and preserved
caller-owned policy at the call sites. Step 3 review found the route aligned
with no testcase-overfit or expectation rewrite.

Final proof recorded in `todo.md` before closure:

- accepted full-suite baseline for commit `f294c3a4`: 3071 passed, 0 failed
- backend regression guard for the final slice passed with 97 before, 97 after,
  and 0 new failures

## Non-Goals

- Do not redesign BIR memory semantics.
- Do not introduce `MemoryLoweringState`.
- Do not combine this with load/store extraction.
- Do not add per-family headers.
- Do not perform testcase-shaped special casing.
