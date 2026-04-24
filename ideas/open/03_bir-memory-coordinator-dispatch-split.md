# BIR Memory Coordinator Dispatch Split

## Intent

Split the oversized `lower_scalar_or_local_memory_inst` coordinator into
smaller instruction-family handlers while preserving the LLM-friendly
`lowering.hpp` index and the fixed memory header budget.

## Rationale

`src/backend/bir/lir_to_bir/memory/coordinator.cpp` currently mixes scalar
lowering, local memory lowering, global provenance, pointer-int address
tracking, dynamic array projections, load/store materialization, and GEP
dispatch. That makes each new memory capability hard to review because the
control-flow boundary is too broad.

Earlier ideas should first extract the memory type/helper vocabulary and
normalize state layering. Once that vocabulary exists, this initiative can move
instruction-family handling out of the monolithic coordinator without a large
semantic rewrite.

## Constraints

- Do not add new `.hpp` files.
- Continue using:
  - `src/backend/bir/lir_to_bir/lowering.hpp`
  - `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
  - `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- Keep `lowering.hpp` as the private index for method declarations.
- New `.cpp` files are allowed only when they represent a real instruction
  family boundary.
- Do not move state ownership out of `BirFunctionLowerer`.

## Design Direction

Keep a thin coordinator:

```text
lower_scalar_or_local_memory_inst
  dispatches to scalar, cast/address-int, alloca, gep, load, store, and
  intrinsic/memory-family handlers
```

The split should be incremental. Each slice should move one instruction family
or one tightly coupled pair, then compile and run the relevant narrow tests.

## Candidate Split Order

1. Separate scalar-only cases from memory coordinator control flow.
   - Scalar compare/select/binop/cast cases should not be buried inside the
     memory coordinator when they do not depend on memory state.

2. Split alloca/local-slot declaration handling.
   - Keep local slot creation, aggregate slot declaration, and scratch slot
     setup in one coherent local memory family.

3. Split GEP handling.
   - Route local slot GEP, local aggregate GEP, global GEP, and dynamic array
     projection through family helpers that use the shared memory vocabulary.

4. Split load/store handling.
   - Keep pointer provenance, local slot materialization, and dynamic array
     load/store paths explicit.
   - Avoid adding testcase-shaped branches.

5. Split runtime memory intrinsic handling if still coupled.
   - `memcpy`, `memset`, and related intrinsic paths should reuse local/global
     memory helpers without expanding the coordinator again.

## Acceptance Criteria

- `lower_scalar_or_local_memory_inst` becomes a thin dispatcher instead of a
  monolithic implementation body.
- Instruction-family handlers are easier to review independently.
- No new `.hpp` files are created.
- `lowering.hpp` remains the complete private index for the lowerer.
- BIR output and diagnostics are unchanged except for intentional bug fixes
  split into separate ideas.
- `c4c_codegen` builds after each moved family.
- Relevant BIR/LIR-to-BIR tests pass with no expectation rewrites.

## Non-Goals

- Do not redesign BIR memory semantics.
- Do not introduce a standalone memory state owner.
- Do not combine this with helper/header extraction.
- Do not downgrade or weaken testcase expectations.
- Do not accept named-case shortcuts as progress.
