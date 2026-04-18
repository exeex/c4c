# LIR To BIR Memory Semantic Ownership Split

Status: Open
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

Continue splitting `lir_to_bir_memory.cpp` into smaller translation units
aligned with semantic ownership while preserving behavior and keeping
`lower_scalar_or_local_memory_inst` as the main coordinator.

## Core Rules

- do not change backend semantics as the claimed outcome of this idea
- do not split by test case, packet name, or equal line counts
- do not force a fake `local` versus `global` divide when helpers are shared
- keep the public lowering contract in `src/backend/bir/lir_to_bir.hpp`
  coherent and reviewable

## Current State

The first ownership-layer split has already landed:

- keep `src/backend/bir/lir_to_bir_memory.cpp` as the opcode-lowering
  coordinator
- add `src/backend/bir/lir_to_bir_memory_addressing.cpp`
- add `src/backend/bir/lir_to_bir_memory_provenance.cpp`

That was a valid first pass, but not the final desired granularity. The current
result still leaves `src/backend/bir/lir_to_bir_memory.cpp` too large, which
means several non-coordinator ownership buckets are still trapped in the main
TU. This idea is reopened to finish that semantic split instead of treating the
first extraction as sufficient end state.

The reopening target remains refactor-only. It should not claim semantic
repair, new backend capability, or changed admission behavior.

## Proposed Ownership Seams

The first ownership layer remains valid:

- keep `src/backend/bir/lir_to_bir_memory.cpp` as the opcode-lowering
  coordinator
- keep `src/backend/bir/lir_to_bir_memory_addressing.cpp` for layout and
  address-walking helpers
- keep `src/backend/bir/lir_to_bir_memory_provenance.cpp` for pointer
  provenance and addressed-object reasoning

The reopened work adds a second ownership layer so the main coordinator TU
stops carrying value synthesis and local-slot mechanics that do not belong
there.

### 1. Coordinator Stays In `lir_to_bir_memory.cpp`

Keep the main opcode admission surface in `lir_to_bir_memory.cpp`, especially
`lower_scalar_or_local_memory_inst`, plus only the thin glue helpers that are
still best read next to that coordinator.

This file should continue to answer:

- which memory-related opcode family is being lowered
- which extracted helper family is consulted
- where backend lowering errors are surfaced for the opcode path

The reopened route should move more helpers out of this file rather than
leaving it as a broad catch-all after the first split.

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

Provenance must remain a real ownership bucket, not a thin leftover bucket that
only exists because other helpers were easier to move first. If more
provenance-bearing helpers are still sitting in `lir_to_bir_memory.cpp`, move
them here before creating unrelated micro-files.

### 4. Value Materialization Bucket

The reopened route should add a dedicated value-materialization ownership
bucket, likely as `src/backend/bir/lir_to_bir_memory_value_materialization.cpp`
unless read-through proves an adjacent existing file is a better fit.

Concrete first-pass candidates include:

- `lower_zero_initializer_value`
- `lower_repeated_byte_initializer_value`
- `lower_typed_index_value`
- `make_index_immediate`
- `synthesize_value_array_selects`
- `synthesize_pointer_array_selects`

This bucket owns value synthesis and initializer materialization helpers that
do not belong in the coordinator merely because memory opcodes consult them.

### 5. Local-Slot / Local-Aggregate Mechanics Bucket

The reopened route should also add a local-slot or local-aggregate mechanics
bucket, likely as `src/backend/bir/lir_to_bir_memory_local_slots.cpp` unless
ownership review finds a better adjacent home.

Concrete first-pass candidates include:

- `collect_local_scalar_array_slots`
- `is_local_array_element_slot`
- `parse_local_array_type`
- `resolve_local_aggregate_gep_slot`
- local dynamic aggregate access helpers
- local dynamic pointer-array access helpers

This bucket should own local-slot naming, array-slot interpretation, and local
aggregate mechanics that are currently too specific to live in pure addressing
but still too structural to stay inside the opcode coordinator.

If ownership is ambiguous for a helper, prefer the semantically strongest of
these buckets over leaving it in `lir_to_bir_memory.cpp` by default. Only keep
helpers in the coordinator when they are genuinely orchestration-specific.

## Primary Targets

- `src/backend/bir/lir_to_bir_memory.cpp`
- `src/backend/bir/lir_to_bir.hpp`
- `src/backend/bir/lir_to_bir_memory_addressing.cpp`
- `src/backend/bir/lir_to_bir_memory_provenance.cpp`
- likely follow-on files such as
  `src/backend/bir/lir_to_bir_memory_value_materialization.cpp` and
  `src/backend/bir/lir_to_bir_memory_local_slots.cpp`
- adjacent `src/backend/bir/lir_to_bir_*.cpp` files only when needed to keep
  helper ownership coherent

## Acceptance Criteria

- [ ] helper families are extracted by semantic ownership rather than arbitrary
      slicing
- [ ] `lower_scalar_or_local_memory_inst` remains the coordinator in the main
      memory-lowering TU
- [ ] the reopened route leaves `lir_to_bir_memory.cpp` as a real coordinator
      rather than a still-monolithic fallback bucket
- [ ] provenance remains a substantive ownership bucket instead of a thin
      residue file
- [ ] value materialization and local-slot or local-aggregate mechanics are
      given explicit homes when read-through confirms the candidate buckets
- [ ] each extraction packet is behavior-preserving move/extract only and does
      not claim semantic repair
- [ ] no backend capability claim depends on this refactor alone
- [ ] the build and a backend-focused proof surface pass after the split

## Validation

- `cmake --build --preset default`
- at minimum, the narrow backend proof surface active when this idea is
  activated
- if the extraction changes shared lowering reach beyond the immediate seam,
  broaden proof rather than treating a tiny packet-local subset as sufficient
- the reopened route should validate each second-layer extraction packet as a
  refactor packet only after the build and the chosen backend proof surface
  both pass

## Non-Goals

- solving the x86 c-testsuite fail surface directly
- de-headerizing `src/backend/mir/x86/codegen/x86_codegen.hpp`
- AArch64 cleanup or parity work

## First-Pass History

- The first ownership-layer split already created
  `src/backend/bir/lir_to_bir_memory_addressing.cpp` and
  `src/backend/bir/lir_to_bir_memory_provenance.cpp`.
- That first pass proved the seam is real, but it did not finish the job:
  `src/backend/bir/lir_to_bir_memory.cpp` remains too large and still holds
  value-materialization and local-slot mechanics that should move into clearer
  ownership buckets.
- Reopening this idea does not erase that history. It records that the first
  split was only the initial layer, not final completion.
