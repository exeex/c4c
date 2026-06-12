Status: Active
Source Idea Path: ideas/open/227_phase_e3_branch_target_helper_oracle_follow_up.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Validate Existing Agreement-Gated Evidence Path

# Current Packet

## Just Finished

Completed Step 1 - Locate The Row Family And Proof Surface.

Exact row family:

- Selected helper-oracle family is the non-compare conditional branch target
  path in `find_prepared_conditional_branch_facts(...)`, where
  `has_compare_branch_facts == false`.
- Current behavior seeds `facts.targets` from
  `PreparedBranchCondition::{true_label,false_label}`, then if prepared
  control-flow has a conditional target row it uses
  `find_prepared_control_flow_branch_target_labels(...)`; if private BIR
  successor evidence agrees it uses
  `read_agreeing_bir_branch_target_labels(...)`; otherwise it keeps the
  prepared control-flow targets.
- This covers materialized-bool conditional branch rows with no compare
  predicate/type/lhs/rhs facts, not the fused-compare row and not
  materialized compare join retargeting.

Implementation target files:

- `src/backend/mir/aarch64/codegen/comparison.cpp`: selected consumer row in
  `find_prepared_conditional_branch_facts(...)`, currently around lines
  147-169; downstream target application is
  `apply_prepared_conditional_branch_targets(...)`.
- `src/backend/prealloc/control_flow.hpp`: helper family definitions,
  `find_prepared_control_flow_branch_target_labels(...)`,
  `detail::BranchTargetIdentityPassContext`, and
  `detail::read_agreeing_bir_branch_target_labels(...)`.

Relevant callers and surfaces:

- `find_prepared_fused_compare_branch_facts(...)` calls
  `find_prepared_conditional_branch_facts(...)` but only after this row has
  chosen branch targets.
- `lower_prepared_conditional_branch_terminator(...)` consumes the facts,
  validates the raw terminator, and calls
  `apply_prepared_conditional_branch_targets(...)` before emission.
- Public prepared helper fallback remains the two-argument
  `find_prepared_control_flow_branch_target_labels(function_cf, block_label)`;
  the three-argument overload with `bir::Block` first gets prepared labels and
  falls back to them if the private agreement reader rejects.

Evidence path:

- `BranchTargetIdentityPassContext` currently carries
  `const bir::Block* source_block`.
- `read_agreeing_bir_branch_target_labels(...)` returns structured BIR
  `{true_label_id,false_label_id}` only when the source block exists, the BIR
  terminator is `CondBranch`, both ids are valid, and both ids equal the
  prepared labels.
- Absent context, invalid ids, mismatched ids, and non-conditional BIR all
  return `std::nullopt`, leaving the caller to preserve prepared fallback.

Existing positive and fallback tests:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`:
  `verify_control_flow_branch_target_labels_use_agreeing_structured_ids()`
  covers prepared-only lookup, private absent context rejection, private
  agreeing structured ids, raw label text drift with agreeing ids, absent
  prepared block, public three-arg agreeing lookup, public fallback on raw text
  drift, public fallback on invalid id, public fallback on mismatch, private
  invalid id rejection, private conflicting id rejection, private
  non-conditional rejection, public fallback on non-conditional BIR, and
  invalid prepared source label rejection.
- `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`:
  `direct_dispatch_lowers_materialized_bool_conditional_branch_to_selected_node()`
  covers the selected non-compare/materialized-bool AArch64 row under normal
  prepared/BIR agreement.
- `tests/backend/mir/backend_aarch64_prepared_branch_records_test.cpp`:
  `conditional_branch_record_preserves_materialized_bool_facts()` covers the
  record-level target pair for the same materialized-bool branch shape.
- Nearby same-feature coverage exists for fused/compare branches in
  `backend_aarch64_branch_control_lowering_test.cpp`
  (`direct_dispatch_lowers_fused_compare_conditional_branch_to_selected_node()`,
  `materialized_compare_branch_stale_prepared_lookup_uses_bir_fallback()`) and
  for compare branch records in
  `backend_aarch64_prepared_branch_records_test.cpp`
  (`conditional_branch_record_preserves_fused_compare_facts()`).

Proof gaps before implementation:

- No selected AArch64 consumer-boundary test currently isolates absent
  `context.bir_block` for this row; helper-level absent context coverage exists
  only in `backend_prepared_lookup_helper`.
- No selected AArch64 row test mutates invalid BIR successor ids, mismatched
  ids, conflicting/duplicate-like ids, or non-conditional BIR and proves the
  emitted branch payload remains prepared.
- No selected AArch64 row test proves public two-argument prepared helper
  fallback is byte-stable after the consumer change.
- No selected AArch64 row test distinguishes prepared control-flow labels from
  raw BIR label text while structured ids still agree; helper-level proof
  exists, but row-level output proof is missing.
- Duplicate/conflict coverage is represented by a private conflicting-id case;
  there is no richer duplicate-row source for this helper because the context
  is a single `bir::Block*` rather than an indexed multi-record table.
- Nearby same-feature coverage exists, but Step 3 should add at least one
  focused selected-row fallback matrix test so progress is not only inferred
  from helper-unit coverage plus normal dispatch.

Lifecycle repair:

- The active plan was repaired after Step 1 found that the selected row already
  calls `prepare::detail::read_agreeing_bir_branch_target_labels(...)`.
- The former Step 2 implementation packet was stale/no-op and is now replaced
  by validation of the existing agreement-gated path.
- Remaining work is selected-row fallback and nearby same-feature proof, not
  adding an already-present evidence path.

## Suggested Next

Step 2 should validate that the existing selected non-compare/materialized-bool
conditional branch row already consumes the private
`BranchTargetIdentityPassContext` /
`read_agreeing_bir_branch_target_labels(...)` path exactly under prepared
agreement, then name the focused selected-row fallback proof to add in Step 3.
Do not perform a no-op implementation change for the already-present reader
call. Suggested narrow proof command if the supervisor delegates execution:

`cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_branch_control_lowering_test backend_aarch64_prepared_branch_records_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering|backend_aarch64_prepared_branch_records)$' --output-on-failure`

## Watchouts

- Keep the slice to the non-compare/materialized-bool conditional branch row;
  do not retarget fused-compare, materialized compare join, unconditional
  branch, wrapper, printer, branch spelling, edge-copy, target-policy, or
  emitted-output surfaces.
- The private reader is fail-closed and has no duplicate-row table; treat
  duplicate/conflict proof as conflicting structured ids unless the supervisor
  requests a separate indexed-context design.
- Preserve prepared fallback for absent context, invalid ids, conflict,
  mismatch, non-conditional BIR, non-agreement, and prepared-only paths.
- Do not rewrite expected strings, baselines, wrapper output, printer/debug
  output, branch-control output, branch spelling, edge-copy scheduling, target
  policy, or emitted-output behavior.
- Reject testcase-shaped matching or fixture-name shortcuts.

## Proof

Discovery-only lifecycle packet. No build/test required by the delegated
packet. Ran symbol/text inspection only; `git diff --check -- todo.md` and
`git status --short` are the local proof commands for this packet.
