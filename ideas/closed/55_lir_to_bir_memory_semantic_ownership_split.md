# LIR To BIR Memory Semantic Ownership Split

Status: Complete
Last Updated: 2026-04-18

## Why This Idea Exists

`src/backend/bir/lir_to_bir_memory.cpp` has grown into a `4926`-line monolith
while the neighboring `lir_to_bir_*` files are already split by lowering
responsibility. Recent x86 local-memory work keeps widening this same file,
which now mixes several semantic ownership domains in one translation unit.

This is not a line-count cleanup. It is a route-quality issue for continued
backend capability work.

## Source Evidence

Reviewer report:

- [review/lir_to_bir_memory_split_review.md](/workspaces/c4c/review/lir_to_bir_memory_split_review.md)

Key conclusions from that review:

- the file now mixes shared aggregate/address-walking helpers, pointer
  provenance, and opcode-lowering coordination
- further local-memory family work will keep forcing unrelated edits into the
  same TU unless those ownership seams are separated
- the defensible split is semantic ownership, not `local` versus `global` and
  not arbitrary line-count slicing

## Goal

Split `lir_to_bir_memory.cpp` into smaller translation units aligned with
semantic ownership while preserving behavior and keeping
`lower_scalar_or_local_memory_inst` as the main coordinator.

## Core Rules

- do not change backend semantics as the claimed outcome of this idea
- do not split by test case, packet name, or equal line counts
- do not force a fake `local` versus `global` divide when helpers are shared
- keep the public lowering contract in `src/backend/bir/lir_to_bir.hpp`
  coherent and reviewable

## Proposed Ownership Seams

The first split should create exactly three translation units:

- keep `src/backend/bir/lir_to_bir_memory.cpp` as the opcode-lowering
  coordinator
- add `src/backend/bir/lir_to_bir_memory_addressing.cpp`
- add `src/backend/bir/lir_to_bir_memory_provenance.cpp`

The first packet is a pure move/extract packet. It should not claim semantic
repair, new backend capability, or changed admission behavior.

### 1. Coordinator Stays In `lir_to_bir_memory.cpp`

Keep the main opcode admission surface in `lir_to_bir_memory.cpp`, especially
`lower_scalar_or_local_memory_inst`, plus only the thin glue helpers that are
still best read next to that coordinator.

This file should continue to answer:

- which memory-related opcode family is being lowered
- which extracted helper family is consulted
- where backend lowering errors are surfaced for the opcode path

### 2. `lir_to_bir_memory_addressing.cpp`

Candidates include:

- aggregate-layout walking
- repeated aggregate extent lookup
- pointer-array length discovery
- byte-offset and relative `gep` target resolution
- shared local/global address walkers that reason about layout rather than
  provenance
- concrete first-pass candidates such as:
  - `can_reinterpret_byte_storage_view`
  - `find_repeated_aggregate_extent_at_offset`
  - `find_pointer_array_length_at_offset`
  - `resolve_global_gep_address`
  - `resolve_relative_global_gep_address`
  - `resolve_local_aggregate_gep_slot`
  - the dynamic aggregate or pointer-array access resolvers that only answer
    layout and offset questions
  - `LocalAggregateGepTarget`-related resolvers

### 3. `lir_to_bir_memory_provenance.cpp`

Candidates include:

- pointer/global alias tracking
- pointer-slot reload provenance
- addressed-object resolution
- honest pointer-base recovery
- pointer-value collection helpers used across local-memory lowering
- concrete first-pass candidates such as:
  - `collect_local_pointer_values`
  - `collect_global_array_pointer_values`
  - `record_pointer_global_object_alias`
  - `resolve_pointer_store_address`
  - `resolve_local_aggregate_pointer_value_alias`
  - `lower_call_pointer_arg_value`
  - `resolve_honest_pointer_base`
  - `resolve_honest_addressed_global_access`
  - `ensure_local_scratch_slot`

If ownership is ambiguous for a helper, prefer keeping it in
`lir_to_bir_memory.cpp` for the first packet instead of inventing a premature
micro-split.

## Primary Targets

- `src/backend/bir/lir_to_bir_memory.cpp`
- `src/backend/bir/lir_to_bir.hpp`
- adjacent `src/backend/bir/lir_to_bir_*.cpp` files only when needed to keep
  helper ownership coherent

## Acceptance Criteria

- [ ] helper families are extracted by semantic ownership rather than arbitrary
      slicing
- [ ] `lower_scalar_or_local_memory_inst` remains the coordinator in the main
      memory-lowering TU
- [ ] the first packet is a behavior-preserving move/extract only and does not
      claim semantic repair
- [ ] no backend capability claim depends on this refactor alone
- [ ] the build and a backend-focused proof surface pass after the split

## Validation

- `cmake --build --preset default`
- at minimum, the narrow backend proof surface active when this idea is
  activated
- if the extraction changes shared lowering reach beyond the immediate seam,
  broaden proof rather than treating a tiny packet-local subset as sufficient
- the first packet should be accepted as a refactor packet only after the build
  and the chosen backend proof surface both pass

## Non-Goals

- solving the x86 c-testsuite fail surface directly
- de-headerizing `src/backend/mir/x86/codegen/x86_codegen.hpp`
- AArch64 cleanup or parity work

## Completion Notes

- The memory-lowering split now lands across three translation units:
  `src/backend/bir/lir_to_bir_memory.cpp`,
  `src/backend/bir/lir_to_bir_memory_addressing.cpp`, and
  `src/backend/bir/lir_to_bir_memory_provenance.cpp`.
- The extracted helper families follow the intended semantic seams:
  addressing/layout helpers live in the addressing TU, pointer-provenance
  helpers live in the provenance TU, and
  `lower_scalar_or_local_memory_inst` remains the coordinator in
  `lir_to_bir_memory.cpp`.
- The final Step 4 cleanup kept the refactor behavior-preserving by consolidating
  shared scalar-subobject admission checks inside the coordinator TU instead of
  widening the route into new capability work.
- Close-time regression guard passed on the backend proof surface with
  `cmake --build --preset default` and
  `ctest --test-dir build -j --output-on-failure -R "^backend_"`.
- The backend subset remains at the same inherited baseline:
  68 passed, 4 failed, 72 total, with no new regressions relative to the
  pre-close baseline.
