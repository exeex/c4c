# BIR Memory Helper Consolidation

## Intent

Consolidate duplicated memory helper logic after the fixed memory vocabulary
headers exist.

This initiative should merge equivalent layout walking, byte-offset projection,
scalar leaf lookup, and scalar subobject checks across
`addressing.cpp`, `provenance.cpp`, and `local_slots.cpp`. It should not split
`coordinator.cpp` and should not move memory state ownership away from
`BirFunctionLowerer`.

## Rationale

The first header extraction creates `memory_types.hpp` and
`memory_helpers.hpp` as the only new memory headers. Once those headers exist,
the next pressure point is duplicated helper behavior, not state ownership.

Today, nearby memory files independently reason about related concepts:

```text
aggregate layout walking
byte offsets through arrays and structs
scalar leaf lookup
scalar subobject addressability
byte-storage reinterpretation
repeated aggregate extents
pointer array lengths at offsets
```

Those should use shared helpers where the semantics are the same. Keeping this
merge separate from the later coordinator split makes behavior review easier:
this idea changes helper factoring, while the next idea changes dispatch shape.

## Constraints

- Do not create additional `.hpp` files.
- Use only:
  - `src/backend/bir/lir_to_bir/lowering.hpp`
  - `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
  - `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- Keep `BirFunctionLowerer` as the state owner.
- Preserve `lowering.hpp` as the full private lowerer index.
- Do not introduce `MemoryLoweringState`.
- Do not split `coordinator.cpp`.

## Refactoring Steps

1. Inventory equivalent helpers.
   - Compare layout walking and offset projection in `addressing.cpp`,
     `provenance.cpp`, and `local_slots.cpp`.
   - Identify helpers that differ only by return shape or naming.
   - Keep genuinely different semantics separate.

2. Merge scalar leaf and scalar subobject reasoning.
   - Consolidate helpers like scalar leaf lookup by byte offset and scalar
     subobject addressability when they are checking the same layout facts.
   - Prefer small result structs over duplicated partial computations.

3. Merge repeated aggregate and pointer-array extent helpers.
   - Reuse common traversal code for repeated aggregate extent lookup and
     pointer-array length-at-offset lookup where the traversal is equivalent.
   - Keep caller-specific policy at the caller.

4. Normalize byte-storage reinterpretation checks.
   - Centralize the common "can this byte storage view be interpreted as this
     target type" logic.
   - Keep the helper explicit about type declarations and target offsets.

5. Keep helpers pure and argument-driven.
   - Helpers in `memory_helpers.hpp` should operate on explicit inputs.
   - Avoid helpers that reach into mutable `BirFunctionLowerer` state.
   - If a helper must mutate alias/provenance maps, leave it in the relevant
     implementation file for now.

6. Preserve behavior.
   - No BIR semantic changes.
   - No expectation rewrites.
   - No coordinator dispatch split in this idea.

## Acceptance Criteria

- Duplicate layout/projection helper logic is reduced across memory files.
- Shared helpers live behind `memory_helpers.hpp` and are used where semantics
  match.
- Caller-specific policy remains in the caller instead of being hidden in a
  generic helper.
- No new headers beyond `memory_types.hpp` and `memory_helpers.hpp` are added.
- `c4c_codegen` builds.
- Relevant BIR/LIR-to-BIR tests pass with no expectation rewrites.

## Non-Goals

- Do not move state ownership out of `BirFunctionLowerer`.
- Do not split `coordinator.cpp`.
- Do not introduce per-family headers.
- Do not redesign address provenance semantics.

## Closure Note

Closed after the active runbook completed all helper consolidation and boundary
validation steps. Duplicate scalar layout facts, aggregate byte-offset
projection, and byte-storage reinterpretation checks were consolidated behind
`memory_helpers.hpp` where semantics matched. Caller-specific policy and mutable
map/provenance updates remain in the implementation files, `BirFunctionLowerer`
remains the memory state owner, and no headers beyond `memory_types.hpp` and
`memory_helpers.hpp` were introduced.

Close validation used the backend subset:
`cmake --build --preset default --target c4c_backend && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
Regression guard passed with 97 passed before and after, 0 failed, using
`--allow-non-decreasing-passed`.
