Status: Active
Source Idea Path: ideas/open/227_phase_e3_branch_target_helper_oracle_follow_up.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Validate Existing Agreement-Gated Evidence Path

# Current Packet

## Just Finished

Completed Step 2 - Validate Existing Agreement-Gated Evidence Path.

Validation result: the existing selected-row implementation satisfies the
agreement-gated evidence contract; no semantic code defect was found and no
implementation/test/log file was edited.

Confirmed implementation facts:

- `find_prepared_conditional_branch_facts(...)` seeds
  `facts.targets` from `PreparedBranchCondition::{true_label,false_label}`
  before any BIR evidence is considered.
- The selected row is the non-compare/materialized-bool path where
  `has_compare_branch_facts == false`; the fused-compare path still exits this
  row and uses `resolve_prepared_compare_branch_target_labels(...)`.
- In that selected row, prepared control-flow targets are read with the public
  two-argument
  `find_prepared_control_flow_branch_target_labels(function_cf, block_label)`.
- The row replaces `facts.targets` with BIR structured successor ids only when
  `prepare::detail::read_agreeing_bir_branch_target_labels(...)` returns
  labels for `BranchTargetIdentityPassContext{.source_block =
  context.bir_block}` and those ids agree with the prepared labels.
- If the private reader returns `std::nullopt`, the selected row keeps the
  prepared control-flow targets; if no prepared control-flow target row exists,
  it keeps the original branch-condition target seed.
- `apply_prepared_conditional_branch_targets(...)` then copies the selected
  `facts.targets` into the emitted branch record target pair.

Confirmed fail-closed boundary:

- Absent consumer context (`context.function.prepared == nullptr`,
  `context.function.bir_function == nullptr`, or `context.bir_block ==
  nullptr`) returns the prepared branch-condition target seed without entering
  the BIR reader path.
- Private absent context, invalid ids, mismatched ids, conflicting structured
  ids, non-conditional BIR terminators, and non-agreement all fail closed at
  `read_agreeing_bir_branch_target_labels(...)` by returning `std::nullopt`.
- The public three-argument helper preserves the prepared fallback when the
  private reader rejects; the public two-argument prepared helper remains
  available and is the selected row's prepared target source.
- Raw BIR label text drift does not override prepared labels when structured
  ids agree; helper-level coverage already proves this for the reader and
  three-argument helper.

Step 3 proof additions to make:

- Add a focused selected-row AArch64 consumer-boundary test in
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp` near
  `direct_dispatch_lowers_materialized_bool_conditional_branch_to_selected_node()`
  that mutates `prepared_with_materialized_bool_conditional_branch()` so BIR
  raw `true_label`/`false_label` text drifts while structured successor ids
  still agree, then asserts the emitted selected branch payload and block
  successors preserve the prepared `block_cf.true_label` and
  `block_cf.false_label`.
- Add a focused selected-row fallback matrix test in the same file, reusing the
  materialized-bool fixture, that proves prepared branch payloads are retained
  for at least invalid structured successor id, mismatched id, conflicting
  structured ids, and non-conditional BIR terminator cases. The helper-level
  absent-context rejection is already covered; consumer-level absent
  `context.bir_block` is not naturally reachable through
  `dispatch_prepared_block(...)` without lower-level context construction, so
  include it only if Step 3 introduces a small local lowering helper.
- Keep nearby same-feature proof mapped to existing tests:
  `direct_dispatch_lowers_fusable_compare_branch_to_selected_node()`,
  `materialized_compare_branch_stale_prepared_lookup_uses_bir_fallback()`,
  `direct_dispatch_lowers_loop_header_fused_compare_branch_with_divergent_bir_label_ids()`,
  and
  `tests/backend/mir/backend_aarch64_prepared_branch_records_test.cpp`'s
  `conditional_branch_record_preserves_fused_compare_facts()`.

## Suggested Next

Step 3 should add selected-row proof only: raw label text drift with agreeing
structured ids, plus a fallback matrix for invalid id, mismatch, conflict, and
non-conditional BIR on the materialized-bool AArch64 consumer row. Suggested
narrow proof command if the supervisor delegates execution:

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
- Do not add code for the agreement reader path in Step 3; Step 2 validated it
  is already present.
- Do not rewrite expected strings, baselines, wrapper output, printer/debug
  output, branch-control output, branch spelling, edge-copy scheduling, target
  policy, or emitted-output behavior.
- Reject testcase-shaped matching or fixture-name shortcuts.

## Proof

Validation/discovery lifecycle packet. No build/test required by the delegated
packet. Ran AST-backed symbol inspection with `c4c-clang-tool-ccdb` plus
focused source/test inspection. Local proof commands for this packet:

- `git diff --check -- todo.md`
- `git status --short`
