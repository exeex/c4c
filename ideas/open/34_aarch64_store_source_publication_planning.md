# AArch64 Store Source Publication Planning

## Goal

Move target-neutral store-source selection facts currently mixed into
`memory_store_sources.*` out of AArch64 codegen and into shared prepare, BIR,
or MIR publication planning.

## Why This Exists

The AArch64 layout classification found that
`src/backend/mir/aarch64/codegen/memory_store_sources.cpp` and
`src/backend/mir/aarch64/codegen/memory_store_sources.hpp` mix real AArch64
store emission with target-neutral source-publication decisions. Those
decisions include narrow-store recovery, byval frame-slot load sources,
cast/select/scalar-FP producer publication, direct global-load select chains,
and pending store-global stack publications. The semantic source choice should
not live behind an AArch64-only helper.

This idea owns the semantic migration only. Any later file consolidation for
the remaining AArch64 memory emission residue belongs to a separate mechanical
fold-back idea.

## In Scope

- Audit the semantic source-publication facts currently inferred through
  `memory_store_sources.*`.
- Move target-neutral store-source selection, producer-source publication, and
  pending stack-publication planning into shared prepare, BIR, or MIR planning
  as appropriate.
- Keep AArch64 as a consumer of prepared store-source facts for memory
  emission.
- Add focused coverage showing that supported store-source choices are present
  before AArch64 codegen consumes them.
- Preserve fail-closed behavior when shared planning does not provide a safe
  source fact.

## Out Of Scope

- Merging `memory_store_sources.*` into `memory.cpp`.
- General AArch64 memory file consolidation after semantic facts have moved.
- Reworking unrelated memory operations such as dynamic stack adjustment,
  normal load/store instruction spelling, or frame-layout ownership.
- Adding AArch64-only rediscovery of source facts in a differently named
  helper.
- Broad BIR or MIR representation rewrites unrelated to store-source
  publication.

## Acceptance Criteria

- At least one concrete store-source decision previously owned only by
  `memory_store_sources.*` is represented as a shared prepared or MIR/BIR
  planning fact before AArch64 emission.
- AArch64 memory lowering consumes that shared fact instead of rediscovering
  the source from same-block producer shape.
- Unsupported or unprepared store-source cases remain explicit and fail
  closed.
- Tests cover both the accepted prepared-source path and a negative case where
  AArch64 cannot proceed without the shared fact.
- The remaining AArch64-only residue is clearly identifiable for later
  mechanical memory fold-back cleanup.

## Reviewer Reject Signals

- The patch keeps source selection in AArch64 codegen and only renames
  `memory_store_sources.*` helpers.
- The patch matches specific test names, value ids, stack slot ids, producer
  instruction order, or fixture labels instead of publishing semantic
  store-source facts.
- The patch claims semantic progress through expectation rewrites,
  unsupported downgrades, or weaker contracts without explicit user approval.
- The patch mixes this semantic migration with mechanical merging of
  `memory_store_sources.*` into `memory.cpp`.
- The patch creates a second AArch64-local fact table or scans same-block
  producers at emission time while calling the result prepared authority.
- The patch leaves the same missing shared fact behind a new abstraction name.
