# AArch64 Store-Source Semantic Residue Prerequisite

## Goal

Move or eliminate the remaining AArch64-local store-source source-choice
recovery in `memory_store_sources.*` so the later memory fold-back can be a
mechanical ownership cleanup.

## Why This Exists

`ideas/open/39_aarch64_memory_foldback_after_store_source_planning.md` was
activated only after the earlier store-source planning work, but its inventory
found that `memory_store_sources.cpp` still contains semantic source recovery.
Folding that file wholesale into `memory.cpp` would preserve the old
AArch64-local decisions under a new owner and would violate idea 39's own
reject signals.

The blocked helpers include:

- `find_latest_narrow_store_for_wide_local_load`
- `store_local_recovered_narrow_store_source`
- `store_local_value_is_byval_frame_slot_load`
- `store_local_value_is_wide_load_from_narrow_local_store`
- `store_local_value_has_select_producer`
- `store_local_value_has_scalar_fp_binary_producer`
- `select_chain_contains_direct_global_load`
- the same-block producer fallback inside
  `emit_pointer_base_plus_offset_to_register`

These helpers use same-block producer scans, slot/object names, lane suffixes,
producer-kind checks, select-chain inspection, or pointer-base load-local
recovery. Some are guarded by or feed
`prepare::plan_prepared_store_source_publication`, but they are not yet pure
prepared-fact consumption.

## In Scope

- Inventory each remaining AArch64-local source-recovery path and decide
  whether it should be represented by shared prepared store-source facts,
  rejected fail-closed when facts are absent, or left as target-only emission
  mechanics.
- Move durable source-choice authority into target-neutral prepare/BIR/MIR
  planning when the behavior is semantic rather than AArch64 instruction
  spelling.
- Keep AArch64 memory emission helpers as consumers of prepared facts.
- Preserve existing diagnostics, fail-closed behavior, and memory/store-source
  semantics.
- Update focused tests only when they prove the generalized source authority or
  compile fallout from moved declarations.

## Out Of Scope

- Folding `memory_store_sources.*` into `memory.cpp`; that remains idea 39
  after this prerequisite is complete.
- Broad memory lowering rewrites, final assembly printing changes, frame layout
  changes, or stack slot allocation changes.
- Moving unrelated calls, dispatch, comparison, prologue, module compatibility,
  or return helpers.
- Test expectation weakening or unsupported downgrades.

## Acceptance Criteria

- AArch64 memory/store-source code no longer owns semantic source-choice
  recovery for the blocked helper family above.
- Any remaining AArch64 helper in `memory_store_sources.*` is either pure
  emission around prepared facts or deliberately fail-closed when the prepared
  authority is missing.
- Shared prepared store-source planning records enough information for the
  existing AArch64 behavior that is still supported.
- Focused store-source planning and AArch64 memory/dispatch tests pass without
  testcase-shaped shortcuts.
- Idea 39 can resume with a genuinely mechanical fold-back route.

## Reviewer Reject Signals

- The patch claims prerequisite progress by renaming or moving the same
  same-block producer scans into another AArch64 helper.
- The patch preserves local source choice behind a new abstraction name instead
  of moving semantic authority into prepared planning or failing closed.
- The patch special-cases a named store fixture, stack slot spelling, value id,
  select chain, byval shape, global symbol, or lane suffix rather than
  generalizing the source authority.
- The patch downgrades unsupported diagnostics, weakens missing-fact tests, or
  removes negative coverage.
- The patch folds `memory_store_sources.*` into `memory.cpp` before the
  semantic residue is resolved.
