# Prepared Move-Bundle Ambiguous Stack Destination For src/960209-1.c

Status: Open
Type: Prepared move-bundle classifier contract repair
Parent: `ideas/closed/555_rv64_prepared_local_memory_addressing_src_960209_1.md`
Related:
- `ideas/closed/516_rv64_multi_source_prepared_move_bundle_classification.md`
- `ideas/closed/552_prepared_move_bundle_target_shape_authority_gaps.md`
Owning Layer: prepared move-bundle producer and classifier contract before RV64 object emission

## Goal

Decide and repair the prepared contract for the `src/960209-1.c` row now that
RV64 local-memory addressing no longer blocks it and the row reaches:

```text
prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination:
prepared move-bundle classifier rejected ambiguous non-parallel multi-source
stack-destination authority
```

## Why This Exists

The local-memory route for `src/960209-1.c` is complete. The row now exposes a
separate prepared move-bundle classifier blocker. Prior classifier work made
ambiguous non-parallel multi-source stack-destination bundles fail closed, but
this row needs fresh evidence to determine whether the prepared producer should
publish coherent authority or whether this is the correct precise rejection for
the row.

This is not RV64 local-memory addressing work. RV64 object emission must not
guess source ownership, sequencing, or stack-destination authority for an
ambiguous prepared bundle.

## In Scope

- Reproduce the `src/960209-1.c` prepared classifier diagnostic after the
  local-memory repair.
- Inspect the concrete prepared move bundle: source homes, destination home,
  destination stack slot, scalar type and width, move count, ordering,
  `parallel_copy`, and authority facts.
- Compare the row shape against the existing fail-closed classifier contract
  from the closed multi-source prepared move-bundle work.
- Decide the valid owner for this row: producer publishes coherent sequencing
  or parallel-copy authority, producer splits the bundle into independent
  moves, or the classifier rejection remains the correct precise blocker.
- Add or update focused prepared/classifier coverage for the row's semantic
  shape, not for its filename.
- Use the one-row RV64 gcc torture backend scan to prove the row either moves
  past this classifier diagnostic or retains a justified, precise
  producer-owned rejection.

## Out Of Scope

- RV64 object-emission materialization that chooses one source, drops a move,
  invents ordering, or consumes an ambiguous bundle.
- Reopening local-memory addressing for this row unless fresh evidence shows
  the classifier blocker was masking a still-present local-memory failure.
- General parallel-copy scheduling or broad move-bundle redesign beyond the
  minimal row-backed contract.
- Expectation rewrites, unsupported marker edits, allowlist filtering, runtime
  comparison changes, or scan-accounting changes.
- Filename-specific handling for `src/960209-1.c`.

## Acceptance Criteria

- The first ambiguous prepared move bundle in `src/960209-1.c` is classified
  with concrete source home, destination home, authority, ordering, and
  parallel-copy facts.
- The route decides whether this row needs a producer repair or whether the
  existing fail-closed classifier rejection is the correct supported outcome.
- Any accepted repair publishes or consumes explicit semantic authority without
  testcase-shaped matching.
- Focused backend or prepared-classifier coverage proves the accepted contract
  or retained rejection for the semantic shape.
- The one-row `src/960209-1.c` proof no longer reports the same unexplained
  `ambiguous_non_parallel_multi_source_stack_destination` blocker; if a
  precise retained rejection remains, the proof and `todo.md` explain why it
  is the correct owner.
- No gcc_torture expectations, unsupported markers, allowlists, or runtime
  comparison behavior are weakened.

## Reviewer Reject Signals

- Reject RV64 object lowering that materializes the bundle by picking the first
  or last source, dropping a move, or relying on incidental source order.
- Reject fixes that special-case `src/960209-1.c`, one block label, one stack
  slot, one value name, or one instruction shape.
- Reject diagnostic-only, helper-rename, or classification-only changes
  claimed as capability progress while the same unexplained ambiguous bundle
  remains.
- Reject unsupported expectation downgrades, weaker test contracts, allowlist
  filtering, or runtime comparison changes without explicit user approval.
- Reject broad parallel-copy rewrites or unrelated prepared-authority cleanup
  that are not required by this row's first ambiguous stack-destination bundle.
- Reject retaining the exact old failure mode behind a new abstraction name
  without concrete row evidence that the fail-closed rejection is the correct
  supported contract.
