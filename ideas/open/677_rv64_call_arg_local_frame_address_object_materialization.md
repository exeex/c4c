# RV64 Call Arg Local Frame Address Object Materialization

Status: Open
Type: Focused implementation or contract proof
Parent: `ideas/open/675_post_wave_residual_baseline_failures.md`
Related:
- `build/agent_state/675_step1_candidate_delta/summary.md`
- `build/agent_state/648_post656_call_evidence/summary.md`
- `test_baseline.log`
- `test_baseline.new.log`
- `ideas/open/675_post_wave_residual_baseline_failures.md`
Owning Layer: RV64 object-route consumption of
`LocalFrameAddressMaterialization`
Queue Order: 77
Proof Surface:
`backend_cli_riscv64_call_arg_local_frame_address_materialization`

## Goal

Resolve the RV64 object-route divergence for local frame address call
arguments by either aligning object emission with the text-route direct
`addi a0, sp, offset` contract or proving that the two-step object shape is the
intended contract.

## Why This Exists

Step 1 of idea 675 showed that
`backend_cli_riscv64_call_arg_local_frame_address_materialization` still fails
object-byte proof. The text route emits direct frame-address materialization
into the ABI argument register, while object emission materializes through a
saved register and then copies to `a0`.

Prior evidence in
`build/agent_state/648_post656_call_evidence/summary.md` already identified
`arg.source_selection=local_frame_address_materialization`; the remaining
owner is the RV64 object route's consumption of that source-selection contract.

## In Scope

- Inspect the RV64 object-emission path that consumes
  `LocalFrameAddressMaterialization` for call arguments.
- Preserve the prepared/source-selection contract that distinguishes local
  frame address materialization from generic register publication.
- Implement the smallest general object-route repair if direct ABI-register
  materialization is the intended contract.
- Alternatively, produce concrete semantic or ABI evidence that the current
  two-step `mv saved, sp; mv a0, saved` shape is an acceptable object contract.
- Prove the focused row and nearby local-frame-address call-argument shapes
  without relying on expectation churn.

## Out Of Scope

- Changing the text-route contract unless fresh evidence proves it is wrong.
- Repairing or reclassifying the pointer/global-local publication expected-fail
  row; that is owned by idea 676.
- Accepting `test_baseline.new.log` while either new-only row is unresolved.
- Expectation rewrites, unsupported-marker changes, allowlist edits, timeout
  changes, runtime-policy edits, or baseline accounting changes.

## Acceptance Criteria

- The object route either emits the expected direct
  `addi a0, sp, offset` shape for local frame address call arguments, or the
  idea records a reviewed contract proving the existing two-step shape should
  be accepted.
- Focused proof covers
  `backend_cli_riscv64_call_arg_local_frame_address_materialization` and at
  least one nearby same-feature object-route case or negative boundary when
  available.
- The implementation, if any, is driven by the
  `LocalFrameAddressMaterialization` semantic contract rather than by test name
  or expected byte string alone.
- Baseline acceptance is deferred until the paired new-only RV64 row is also
  settled.

## Reviewer Reject Signals

- Reject fixes keyed to
  `riscv64_call_arg_local_frame_address_materialization` by filename, expected
  byte string, or named test identity.
- Reject expectation rewrites, unsupported-marker downgrades, allowlist edits,
  timeout/accounting changes, or weaker object-byte checks as progress.
- Reject helper renames, diagnostic-only changes, or classification-only edits
  claimed as object-route materialization repair.
- Reject broad RV64 call lowering rewrites that are not justified by
  `LocalFrameAddressMaterialization` consumption evidence.
- Reject leaving the text/object divergence unexplained behind a new helper or
  abstraction name.
