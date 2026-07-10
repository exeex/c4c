# AArch64 Instruction Dispatch Internal

Status: Open
Type: Implementation
Parent: `ideas/open/658_backend_baseline_history_umbrella_triage.md`
Related:
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
Owning Layer: AArch64 instruction dispatch and prepared-BIR AArch64
publication
Queue Order: 65
Proof Surface: current baseline rows 284 and 322 from
`log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`.

## Goal

Repair the AArch64-specific instruction dispatch and prepared-BIR publication
path without mixing it into RV64 runtime or generic prepared CLI work.

## Why This Exists

Step 2 kept `backend_aarch64_instruction_dispatch` separate from RV64
families, and also identified a CLI prepared-BIR row with AArch64 publication
in its label. These rows share target-specific AArch64 ownership and should
follow the generic prepared/CLI contract idea only when that broader route
does not already settle row 322.

## In Scope

- Refresh focused AArch64 dispatch and prepared-BIR AArch64 publication
  evidence.
- Identify whether the first owner is AArch64 instruction selection,
  dispatch-table coverage, prepared-BIR AArch64 publication, or target CLI
  exposure of existing facts.
- Repair one AArch64 target-internal rule after the first owner is proven.
- Coordinate with the prepared contract/CLI idea by keeping shared publication
  evidence explicit.

## Out Of Scope

- RV64 runtime lowering, RISC-V object emission, stack fan-in authority, byval
  call-boundary work, object-data static storage, or LLVM torture diagnosis.
- Generic CLI text formatting unless focused evidence proves AArch64 facts
  already exist and only exposure is broken.
- Expectation rewrites, unsupported-marker changes, allowlists, timeout
  changes, runtime policy changes, or baseline acceptance changes.

## Acceptance Criteria

- Focused evidence names the first AArch64 owner for rows 284 and 322, or
  returns row 322 to the generic prepared contract route with concrete proof.
- The selected repair adds or consumes general AArch64 dispatch/publication
  facts rather than matching a named test.
- The focused AArch64 subset passes or fails closed with precise diagnostics.
- Backend regression proof shows no new backend failures in the supervisor's
  chosen subset.

## Reviewer Reject Signals

- Reject merging AArch64 instruction dispatch into RV64 lowering or RISC-V
  object emission.
- Reject named-case dispatch-table shortcuts that only satisfy the current row
  without covering the general missing AArch64 dispatch/publication shape.
- Reject expectation rewrites, unsupported-marker downgrades, allowlist edits,
  helper renames, or classification-only edits claimed as capability progress.
- Reject treating CLI formatting as the only owner while AArch64 dispatch or
  publication facts are missing.
- Reject a patch that leaves the same AArch64 dispatch failure behind a new
  name.
