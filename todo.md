Status: Active
Source Idea Path: ideas/open/219_phase_e1_control_flow_identity_helper_contraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Control-Flow Identity Surface

# Current Packet

## Just Finished

Completed `plan.md` Step 1 selection for one control-flow identity surface.
Selected helper: `find_prepared_control_flow_branch_target_labels(...)`.

Why it is identity-only: the helper returns only a
`PreparedBranchTargetLabels` pair for a conditional branch source block. That
pair is target-neutral successor identity: true successor label and false
successor label. It does not choose branch spelling, condition suffixes,
compare fusion legality, edge-copy scheduling, move bundles, wrappers,
printer/debug rows, diagnostics, or expected strings.

BIR/route identity owner: BIR conditional terminator identity in
`bir::Terminator::true_label_id` and `bir::Terminator::false_label_id`, with
raw `true_label`/`false_label` only as compatibility spelling when structured
ids are absent. No route owner is needed for this selected helper.

Retained prepared fallback/policy owners:

- `PreparedControlFlowFunction` and `PreparedControlFlowBlock` remain the
  fallback owners for absent BIR context, invalid labels, missing prepared
  blocks, non-conditional blocks, BIR/prepared mismatch, and compatibility-only
  raw-label paths.
- `PreparedBranchCondition` and
  `resolve_prepared_compare_branch_target_labels(...)` remain owners for
  predicate/compare facts and agreement checks used by compare-branch readers.
- AArch64 comparison lowering remains owner of branch spelling, condition
  suffix selection, compare fusion policy, diagnostics, and emitted target
  records.
- x86 handoff and short-circuit consumers remain owners of wrapper/output
  behavior and authoritative prepared fallback where raw BIR labels drift.

Current callers inspected:

- `src/backend/prealloc/control_flow.hpp`: helper definition plus
  `resolve_prepared_compare_branch_target_labels(...)` and related
  compare-branch facade helpers.
- `src/backend/mir/aarch64/codegen/comparison.cpp`:
  `find_prepared_conditional_branch_facts(...)` is the direct non-header
  caller found by `c4c-clang-tool-ccdb function-callers`; AST callees show it
  also depends on `find_prepared_branch_condition(...)`,
  `resolve_prepared_compare_branch_target_labels(...)`,
  `find_prepared_materialized_compare_join_targets(...)`,
  `find_prepared_short_circuit_join_context(...)`, and
  `find_prepared_short_circuit_branch_plan(...)`.
- `src/backend/bir/bir.hpp`: BIR `Terminator` exposes structured
  `true_label_id` and `false_label_id` as the semantic block references for
  conditional branches.
- Nearby candidate helpers were inspected and rejected for this packet:
  `find_prepared_control_flow_function(...)` and
  `find_prepared_control_flow_block(...)` return aggregate prepared records,
  `find_prepared_linear_join_predecessor(...)` walks predecessor paths, and
  `resolve_prepared_block_label_id(...)` has only prepared-name-table inputs
  and no narrow BIR agreement point by itself.

Current tests inspected:

- `tests/backend/bir/backend_prepare_structured_context_test.cpp` covers
  structured BIR ids for prepared conditional true/false labels.
- `tests/backend/bir/backend_prepare_block_only_control_flow_test.cpp` covers
  block-only branch target metadata publication.
- `tests/backend/bir/backend_x86_handoff_boundary_local_slot_guard_lane_test.cpp`
  and `tests/backend/bir/backend_x86_handoff_boundary_short_circuit_test.cpp`
  cover prepared authoritative target fallback when raw BIR branch labels drift.
- `tests/backend/mir/backend_aarch64_prepared_branch_records_test.cpp` covers
  target-pair label preservation in AArch64 prepared branch records.

## Suggested Next

Delegate Step 2 for only `find_prepared_control_flow_branch_target_labels(...)`:
add an agreement-gated overload or local reader that accepts the source
`bir::Block` and uses structured BIR conditional terminator
`true_label_id`/`false_label_id` only when they are valid and agree with the
retained prepared helper result; otherwise preserve the current prepared
fallback result.

Recommended narrow proof command for the next code-changing packet:

`cmake --build build --target backend_prepare_structured_context_test backend_prepare_block_only_control_flow_test backend_prepared_lookup_helper_test backend_x86_handoff_boundary_test backend_aarch64_prepared_branch_records_test && ctest --test-dir build -R '^(backend_prepare_structured_context|backend_prepare_block_only_control_flow|backend_prepared_lookup_helper|backend_x86_handoff_boundary|backend_aarch64_prepared_branch_records)$' --output-on-failure`

## Watchouts

- Keep the selected surface to one helper or one reader.
- The selected reader is `find_prepared_control_flow_branch_target_labels(...)`
  only; do not widen into `find_prepared_compare_branch_target_labels(...)`,
  short-circuit branch-plan helpers, join predecessor helpers, branch
  conditions, or aggregate control-flow lookup replacement.
- Do not retire aggregate `PreparedControlFlow`.
- Do not delete public prepared APIs.
- Do not move branch spelling, edge-copy scheduling, move policy, wrappers,
  printer/debug rows, diagnostics, helper-oracle behavior, or expected strings
  into BIR/route authority.
- Keep BIR authority to structured conditional successor identity only. Raw
  label spellings are compatibility fallback and must not override prepared
  authority on mismatch.
- Do not use baseline refreshes, unsupported downgrades, helper renames,
  timeout masking, or expectation rewrites as proof.
- If no candidate can be isolated without policy migration, stop and report
  the lifecycle/route blocker instead of broadening the plan.

## Proof

Inspection-only packet. No build or test proof was required, and no
`test_after.log` update was made because the delegated proof was the
`todo.md` selection grep.
