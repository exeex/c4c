# AArch64 Memory Fold-Back After Store Source Planning

## Goal

Fold remaining AArch64 memory store-source emission residue back into
`memory.cpp` after shared store-source publication planning has been moved out
of `memory_store_sources.*`.

## Why This Exists

The classification table intentionally separated semantic
`memory_store_sources.*` migration from mechanical memory cleanup. Once the
target-neutral source-publication facts are owned by shared prepare, BIR, or
MIR planning, any remaining AArch64-only load/store emission helpers should be
consolidated under the reference-style `memory` owner instead of staying in a
separate helper family.

## In Scope

- After `ideas/open/34_aarch64_store_source_publication_planning.md` has
  moved semantic source facts forward, fold remaining target-local residue
  from:
  - `src/backend/mir/aarch64/codegen/memory_store_sources.cpp`
  - `src/backend/mir/aarch64/codegen/memory_store_sources.hpp`
- Use `memory.cpp` / `memory.hpp` as the AArch64 memory lowering owner.
- Keep only AArch64 instruction spelling, addressing, scratch, and store/load
  emission logic in this mechanical cleanup.
- Update includes and build metadata only as needed for removed helper files.
- Preserve fail-closed behavior for missing shared store-source facts.

## Out Of Scope

- Moving target-neutral store-source facts; that belongs to the semantic
  store-source publication planning idea.
- Reworking `memory_dynamic_stack.*` unless only include fallout is required.
- Changing frame layout, stack slot allocation, or broad memory operation
  semantics.
- Introducing AArch64-local source rediscovery after semantic planning has
  moved forward.
- Folding calls, dispatch, comparison, prologue, or module compatibility
  helpers.

## Acceptance Criteria

- This work begins only after the semantic store-source publication planning
  slice has provided shared facts or explicitly fail-closed selected cases.
- Remaining AArch64-only store-source emission helpers are owned by the
  memory family rather than standalone `memory_store_sources.*` files.
- AArch64 memory lowering consumes shared store-source facts and does not
  rediscover semantic source choices locally.
- Build metadata no longer references removed memory store-source helper
  translation units.
- Validation covers focused AArch64 memory/store-source behavior and an
  appropriate backend build or test bucket.

## Reviewer Reject Signals

- The patch starts mechanical memory fold-back while semantic source choice
  still lives only in AArch64 `memory_store_sources.*`.
- The patch claims progress by moving code into `memory.cpp` but keeps
  target-neutral source selection as local same-block producer scanning.
- The patch special-cases a named store fixture, global symbol, stack slot,
  value id, select chain, or byval aggregate shape.
- The patch weakens unsupported diagnostics, downgrades tests, or removes
  negative coverage for missing shared facts.
- The patch mixes semantic publication migration with this mechanical cleanup
  instead of depending on the separate semantic idea.
