# RISC-V Object Emission Internal Probe

Status: Closed
Type: Implementation
Parent: `ideas/open/658_backend_baseline_history_umbrella_triage.md`
Related:
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
Owning Layer: RISC-V object-emission backend infrastructure
Queue Order: 64
Proof Surface: current baseline row 256 from
`log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`.

## Goal

Classify and repair the current `backend_riscv_object_emission` failure at the
RISC-V object-emission infrastructure layer.

## Why This Exists

Step 2 marked the RISC-V object-emission row as blocked pending probe and
explicitly warned not to merge it into RV64 runtime families without evidence.
This idea owns that probe-first route.

## In Scope

- Refresh focused evidence for RISC-V object emission from prepared/RV64
  lowering through object writer output.
- Identify whether the first owner is relocation emission, section layout,
  symbol publication, instruction encoding, or object writer contract.
- Repair one RISC-V object-emission infrastructure rule only after the focused
  probe names the failing object contract.
- Keep RV64 runtime semantic rows separate unless object emission is proven as
  their first owner.

## Out Of Scope

- RV64 pointer-local, byval, destination-publication, object-data static
  storage, callee-saved GPR, or packed local member runtime repairs.
- AArch64 instruction dispatch, prepared CLI exposure, or LLVM torture owner
  discovery.
- Test expectation rewrites, unsupported-marker changes, allowlists, timeout
  changes, runtime policy changes, or baseline acceptance changes.

## Acceptance Criteria

- Focused probe evidence names the first RISC-V object-emission owner.
- The selected repair preserves existing prepared and RV64 lowering contracts
  unless the probe proves they are stale or missing.
- The focused object-emission row passes or fails closed with a precise
  diagnostic at the proven owner.
- Backend regression proof shows no new backend failures in the supervisor's
  chosen subset.

## Lifecycle Note 2026-07-10

The focused `backend_riscv_object_emission` row now passes after the Step 4
repair sequence, and focused regression evidence under
`build/agent_state/664_step4_local_frame_address_publication/` shows row 256
moving from failed to passed. Closure was not accepted because the
supervisor-reported full-suite baseline candidate regressed from the accepted
3386/3397 baseline to 3384/3397 by adding rows 139 and 176:
`backend_cli_riscv64_pointer_global_local_publication` and
`backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract`.
Those rows are outside this object-emission row-256 source scope, so follow-up
ownership is split to `ideas/open/673_post_664_full_suite_regression_probe.md`
instead of expanding this idea.

## Closure Note 2026-07-10

Closed after refreshing row 256 as passing and reconciling the post-664 split
blockers through closed ideas 673 and 674. Close-readiness proof covered
`backend_riscv_object_emission`,
`backend_cli_riscv64_pointer_global_local_publication`, and
`backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract`; all
three passed before and after, with no new failures. The strict guard only
failed on unchanged pass count, and the lifecycle close guard passed with
`--allow-non-decreasing-passed`.

## Reviewer Reject Signals

- Reject absorbing unrelated RV64 runtime rows into this object-emission route
  without concrete object-writer evidence.
- Reject named-case fixes for `backend_riscv_object_emission`.
- Reject changing expectations, unsupported markers, allowlists, timeouts,
  runtime behavior, or baseline accounting as progress.
- Reject helper renames, diagnostic-only edits, or classification-only changes
  that leave the same object-emission failure.
- Reject using final object bytes as proof when the required object contract is
  not named.
