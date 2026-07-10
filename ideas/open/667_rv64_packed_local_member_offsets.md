# RV64 Packed Local Member Offsets

Status: Open
Type: Implementation
Parent: `ideas/open/658_backend_baseline_history_umbrella_triage.md`
Related:
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
Owning Layer: RV64 packed local member offset lowering
Queue Order: 67
Proof Surface: current baseline row 236 from
`log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`.

## Goal

Repair RV64 lowering for packed local member offsets.

## Why This Exists

Step 2 assigned `backend_rv64_runtime_packed_local_member_offsets` to a
single RV64 packed local member offset owner. It should stay separate from
prepared object-data static storage and broader object-emission work.

## In Scope

- Refresh focused semantic, prepared, RV64 assembly, object, and runtime
  evidence for packed local member offset lowering.
- Identify whether the first owner is packed layout publication, local object
  base selection, member offset arithmetic, load/store width, or RV64 address
  materialization.
- Repair one general packed local member offset rule after proving layout and
  address facts.

## Out Of Scope

- Static local object-data storage, byval payload preservation, pointer-local
  postincrement, callee-saved GPR preservation, destination-publication,
  AArch64, CLI, RISC-V object emission, or LLVM torture diagnosis.
- Test expectation rewrites, unsupported-marker changes, allowlists, timeout
  changes, runtime policy changes, or baseline acceptance changes.

## Acceptance Criteria

- Focused evidence names the first packed local member offset owner.
- The selected repair uses explicit packed layout and local object address
  facts rather than hard-coded offsets or testcase names.
- The focused runtime row passes or fails closed with a precise diagnostic.
- Backend regression proof shows no new backend failures in the supervisor's
  chosen subset.

## Reviewer Reject Signals

- Reject hard-coded offsets, field names, filenames, or named-case matching.
- Reject merging static local object-data or generic object emission into this
  packed-member route without proof.
- Reject expectation rewrites, unsupported-marker downgrades, allowlist edits,
  helper renames, or classification-only edits claimed as progress.
- Reject using final assembly as the only authority when packed layout or
  address facts are missing.
- Reject retaining the same packed member offset failure behind a renamed
  helper or diagnostic.
