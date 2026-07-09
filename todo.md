Status: Active
Source Idea Path: ideas/open/616_select_publication_source_wiring.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Close-Readiness Classification

# Current Packet

## Just Finished

Step 4, `Close-Readiness Classification`, prepared the plan-owner closure
classification for idea `616` from the Step 1-3 evidence.

Close-readiness result: idea `616` acceptance criteria are met for the current
runbook. Step 2 moved multiple complete-authority stack-offset select
publication rows past `unsupported_source_stack_offset`, preserved fail-closed
behavior for rows without source freshness, and passed the delegated backend
proof. Step 3 found no remaining complete-authority select-publication
stack-offset source family that still belongs to this idea.

Residual owner classification:
- `src/20000706-1.c`, `src/20000706-2.c`, `src/20000717-5.c`,
  `src/20071213-1.c`, `src/20120427-1.c`, and `src/20120427-2.c` now reach
  `[RV64_BACKEND_RUNTIME_MISMATCH]`; separate runtime/semantic owner.
- `src/991216-1.c` now fails first on generic
  `unsupported_move_bundle_target_shape`; separate generic move-bundle
  target-shape owner.
- `src/pr45034.c`, `src/pr53160.c`, `src/pr58726.c`, and `src/pr59221.c`
  remain `intent_status_unsupported_source_home`; separate source-home/source
  freshness owner.
- `src/pr29695-1.c` and `src/pr29695-2.c` remain
  `unsupported_source_immediate_i32_range`; separate large-immediate
  materialization/range owner.
- `src/921124-1.c` and `src/920710-1.c` remain
  `unsupported_terminator_fragment`; separate terminator owner.

Recommended lifecycle action for plan owner: close idea `616`. Do not rewrite
or block this plan for the residuals above; if pursued, source-home,
large-immediate, runtime mismatch, generic move-bundle target-shape, and
terminator residuals should be separate owners or follow-up ideas.

## Suggested Next

Plan owner should close idea `616` using this classification, subject to the
supervisor's final lifecycle review and commit boundary.

## Watchouts

- Do not treat `unsupported_source_home` rows as established source freshness.
- Do not widen idea `616` into large-immediate materialization, runtime,
  generic move-bundle target-shape, or terminator work.
- No implementation files, unsupported markers, allowlists, expectations,
  runtime/accounting files, test logs, `plan.md`, or source ideas were changed.

## Proof

Lifecycle/diagnostics classification only. No fresh build or test command was
required, and `test_after.log` was not created or modified for this packet.

Evidence used: existing `plan.md`, source idea
`ideas/open/616_select_publication_source_wiring.md`, Step 2 backend proof
recorded in prior `todo.md` state, and Step 3 targeted diagnostics recorded in
prior `todo.md` state.
