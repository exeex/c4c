Status: Active
Source Idea Path: ideas/open/553_move_bundle_target_shape_evidence_gap_src_960209_1.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Locate The Missing Diagnostic Authority

# Current Packet

## Just Finished

Completed Step 2, "Locate The Missing Diagnostic Authority", by tracing the
fresh `src/960209-1.c` diagnostic to
`src/backend/prealloc/prepared_object_traversal.cpp`. The concrete emitter is
`diagnose_prepared_object_consumer(const PreparedObjectMoveBundleConsumerClassification&)`
at the `PreparedObjectMoveBundleConsumerStatus::MissingMoveBundle` branch,
which constructs `PreparedObjectConsumerDiagnosticCategory::MissingMoveBundle`
with message `prepared copy traversal event is missing move-bundle authority`.

The source path is:
`src/backend/mir/riscv/codegen/object_emission.cpp::prepared_function_to_object_function`
builds a prepared traversal with
`make_prepared_object_function_traversal(...)`, classifies copy events through
`classify_prepared_object_move_bundle_consumer(...)`, returns the diagnostic
category/message, and `src/backend/backend.cpp::emit_rv64_prepared_object_module`
formats it as `prepared_consumer_category=missing_move_bundle`.

This evidence gap is not printing-only. The formatter is faithfully reporting a
structured diagnostic. The missing fact appears producer/fact-propagation-owned:
the classifier reaches a copy traversal event whose `event.move_bundle` is
null. For parallel-copy events, that null is produced by
`find_parallel_copy_move_bundle(...)` when no unique
`PreparedMoveBundle` matches the `PreparedParallelCopyBundle` execution block,
predecessor/successor labels, `BlockEntry` phase, and
`OutOfSsaParallelCopy` authority.

## Suggested Next

Execute Step 3 by adding the smallest diagnostic/evidence surface around the
`src/960209-1.c` failing traversal event: event kind, block index, instruction
index, prepared block label, optional parallel-copy predecessor/successor and
execution labels, whether `event.move_bundle` is null or ambiguous, matching
move-bundle candidate phase/authority/block/predecessor/successor facts, and
the relevant move value/home/type facts, including any F128-related value type
or home facts visible from the prepared lookups.

## Watchouts

- This plan is evidence-first. Do not route the row to RV64, prepared, BIR, or
  F128 from filename, source shape, raw BIR shape, or bucket membership alone.
- Do not change expectations, unsupported markers, allowlists, or runtime
  comparison behavior.
- Keep any diagnostic work focused on emitting auditable facts, not on making a
  narrow testcase pass.
- The current one-row diagnostic supersedes stale docs/tables that classify
  `src/960209-1.c` as `unsupported_move_bundle_target_shape`; the fresh row
  says `prepared_consumer_category=missing_move_bundle`.
- The most likely Step 3 observation point is the
  `prepared_function_to_object_function` copy-event classification loop or the
  `find_parallel_copy_move_bundle(...)` lookup path. Avoid changing source
  semantics until the event/authority coordinates identify which producer fact
  is missing.

## Proof

Ran the delegated Step 2 proof command exactly:

```sh
{ echo '== cmake --build --preset default =='; cmake --build --preset default; } > test_after.log 2>&1
```

The build completed successfully. Proof log preserved at `test_after.log`.
