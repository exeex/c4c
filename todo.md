Status: Active
Source Idea Path: ideas/open/548_prepared_global_stack_frame_infrastructure_review.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Classify Prepared Move-Bundle Ownership

# Current Packet

## Just Finished

Completed plan Step 4 by classifying the two current
`unsupported_prepared_move_bundle_classification` representatives into
prepared classifier authority versus RV64 handoff readiness.

Classifications:

- `src/20010224-1.c`:
  first owner is `prepared_move_bundle_classifier`. The row still stops at
  `unsupported_prepared_move_bundle_classification` with
  `diagnostic_owner=prepared_move_bundle_classifier` and
  `fragment_status=producer_classification_rejected_stack_source_stack_destination_conversion_adjacent_move`.
  The rejected move is a `before_instruction_copies` stack-slot to stack-slot
  conversion-adjacent move in `ba_compute_psd`, block `for.cond.1`, with
  `authority=none`, `i16` source, and `i32` destination. It is not ready for
  RV64 object-route consumption.
- `src/pr87623.c`:
  same first owner as `src/20010224-1.c`. The row stops at
  `prepared_move_bundle_classifier` for a `before_instruction_copies`
  stack-slot to stack-slot conversion-adjacent move in `a_or_b_different`,
  block `logic.end.12`, with `authority=none`, `i8` source, and `i32`
  destination. It is not ready for RV64 object-route consumption.

Scope classification:

- Both rows are retained prepared-classifier work, not RV64 handoff-ready
  object-route work.
- The common missing authority/source-destination form is a conversion-adjacent
  stack-source to stack-destination move where the integer source and
  destination sizes differ.
- This does not look like an expected fail-closed RV64 consumer rejection. The
  prepared classifier has not yet assigned authority to a coherent prepared
  move-bundle shape, so RV64 does not have an admissible object-route handoff.

Evidence artifact:

- `build/agent_state/548_step4_move_bundle_classification/classification.md`

## Suggested Next

Ask the plan owner to reconcile this review plan. Recommended split: a
prepared move-bundle classifier follow-up focused on conversion-adjacent
stack-slot to stack-slot moves with differing integer widths, where the repair
either publishes an authorized widening stack move bundle or lowers/splits the
conversion into an explicit supported prepared form before RV64 consumption.

## Watchouts

- This is a review/classification plan. Do not implement prepared or RV64
  lowering inside this active idea.
- `src/20000412-1.c` and `src/20001121-1.c` should split into separate
  follow-up ideas if implementation proceeds: prepared zero-fill object-data
  producer repair versus RV64 floating global-memory consumption.
- No F128 quarantine was discovered for either global-data row. `20000412-1.c`
  is an ordinary pointer-array global case; `20001121-1.c` uses `double`, not
  F128.
- Stack-frame implementation work should split into an RV64 FPR
  callee-saved-frame follow-up if accepted. Keep ordinary GPR frame work out of
  that scope unless separate evidence identifies a GPR-specific gap.
- The move-bundle rows still name `prepared_move_bundle_classifier` as
  diagnostic owner and `authority=none`; avoid converting them into RV64
  consumer work before the prepared classifier authority gap is repaired.
- Do not change expectations, unsupported markers, allowlists, or pass/fail
  accounting as evidence of progress.

## Proof

Proof/evidence used:

- Existing Step 1 aggregate log:
  `build/agent_state/548_step1_infrastructure_evidence.log`
- Existing per-case log:
  `build/rv64_gcc_c_torture_backend/src_20010224-1.c/case.log`
- Existing per-case log:
  `build/rv64_gcc_c_torture_backend/src_pr87623.c/case.log`
- Focused Step 4 classification artifact:
  `build/agent_state/548_step4_move_bundle_classification/classification.md`

No broad tests were run and no root-level `test_after.log` was written because
the delegated packet explicitly requested existing-log classification only and
forbade root-level proof logs.

Traceable logs:

- Aggregate:
  `build/agent_state/548_step1_infrastructure_evidence.log`
- Per-case:
  `build/rv64_gcc_c_torture_backend/src_20010224-1.c/case.log`
- Per-case:
  `build/rv64_gcc_c_torture_backend/src_pr87623.c/case.log`
