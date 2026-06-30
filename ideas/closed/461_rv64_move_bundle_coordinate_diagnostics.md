# RV64 Move-Bundle Coordinate Diagnostics

Status: Closed
Type: RV64 diagnostic/probe support idea
Parent: `ideas/closed/460_rv64_move_bundle_residual_owner_audit.md`
Source Evidence: `build/agent_state/460_step2_residual_disposition/`
Owning Layer: RV64 move-bundle rejection diagnostics and probe evidence

## Goal

Provide coordinate-bearing RV64 move-bundle rejection evidence for
`unsupported_move_bundle_target_shape` so the next lowering owner can be
selected from proven phase/block/instruction/move identity instead of broad
diagnostic text or raw-shape inference.

## Why This Exists

Idea 460 could not select a semantic implementation packet because current
object diagnostics only report the broad move-bundle rejection class. The
candidate set includes the `20010329-1` before-instruction stack publication
at `block_index=4 instruction_index=1`, the idea-459 suppression target, later
register setup rows, and block-entry select publication copies. The first
failing event must be identified with coordinate-bearing evidence before any
lowering idea is safe.

## In Scope

- Add or define a narrow diagnostic/probe surface that reports the rejecting
  move-bundle phase, block index, instruction index, move index or move
  identity, destination storage, reason, and relevant authority status.
- Re-probe `20010329-1` and capture coordinate-bearing evidence under
  `build/agent_state/461_*`.
- Keep diagnostics/probes stable enough for lifecycle ownership decisions, but
  do not claim semantic lowering progress.
- Add focused diagnostic/probe coverage if code changes are required.
- Use the resulting evidence to route the residual to the exact owner.

## Out Of Scope

- Implementing RV64 move-bundle lowering before coordinate evidence exists.
- Generic stack-to-register or register-to-register move support.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Reopening ideas 456, 458, or 459 without coordinate-bearing evidence that
  their exact route is first failing.
- Inferring ownership from value ids, block indexes, instruction indexes, raw
  BIR shape, filenames, function names, or one prepared dump alone.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, or `test_baseline.new.log` churn.

## Acceptance Criteria

- `20010329-1` object-route failure includes coordinate-bearing evidence for
  the first rejecting move-bundle event, or a documented reason why such
  evidence cannot be produced.
- The evidence distinguishes at least the candidate families identified by
  idea 460: cast-dependency stack publication, idea-459 suppression matcher,
  later register setup rows, block-entry select copies, or another owner.
- No semantic lowering or expectation change is claimed as progress.
- A follow-up source idea, if needed, is specific to the proven event owner and
  preserves fail-closed stale stack-load, generic move support, and raw-shape
  boundaries.

## Completion Notes

Closed by lifecycle split after Step 4 residual disposition. Step 3 added
coordinate-bearing RV64 move-bundle diagnostics, and Step 4 recorded the first
remaining `20010329-1` object-route failure as:

| Field | Value |
| --- | --- |
| Diagnostic class | `unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves` |
| Event kind | `pre_terminator_copies` |
| Function | `main` |
| Block | `block_index=10`, `block_label=logic.rhs.end.40` |
| Instruction | `instruction_index=0` |
| Move-bundle phase | `block_entry` |
| Bundle coordinates | `bundle_block_index=10`, `bundle_instruction_index=0` |
| Bundle authority | `out_of_ssa_parallel_copy` |
| Parallel-copy context | `logic.rhs.end.40 -> logic.end.41`, `execution_site=predecessor_terminator` |
| Move | `move[0].from_value_id=20`, `move[0].to_value_id=21`, `destination_kind=value`, `destination_storage=register`, `op_kind=move`, `reason=phi_join_register_to_register` |
| Suppression authority | `select_edge_suppression_authorized=no` |
| Cast stack publication authority | `cast_dependency_stack_publication_authorized=no` |
| Fragment status | `generic_move_bundle_materialization_failed` |

This satisfies the diagnostics/probe source scope. The residual owner is not
the earlier `block_index=4 instruction_index=1` cast-dependency stack
publication candidate, not the idea-459 suppression matcher target, not stale
stack-load authority, and not a generic unnamed stack/register move bucket.

Follow-up source idea:
`ideas/open/462_rv64_preterminator_predecessor_edge_parallel_copy_materialization.md`.

Close validation used existing canonical regression logs and `git diff --check`;
no implementation, test, review, or baseline-log files were changed by this
lifecycle transition.

## Reviewer Reject Signals

- Reject lowering changes presented as diagnostic/probe support.
- Reject diagnostics that still omit enough phase/block/instruction/move
  identity to select an owner.
- Reject classifying ownership from a single prepared dump without
  object-route rejection evidence.
- Reject consuming `load_from_stack_slot missing_stack_freshness` or adding
  generic stack/register move support.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, or baseline/log churn.
