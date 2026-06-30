# RV64 Move-Bundle Coordinate Diagnostics

Status: Open
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
