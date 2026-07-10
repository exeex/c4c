# Prepared Object Data Static Storage Runtime

Status: Open
Type: Implementation
Parent: `ideas/open/658_backend_baseline_history_umbrella_triage.md`
Related:
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
Owning Layer: prepared object-data/static-storage publication and RV64
object-data consumption
Queue Order: 63
Proof Surface: current baseline rows 183 and 184 from
`log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`.

## Goal

Repair prepared object-data and RV64 runtime handling for static local storage
and initialized static local storage.

## Why This Exists

Step 2 assigned two current object-runtime rows to prepared object-data/static
storage ownership. The family is narrower than the broad prepared/call-boundary
routes but should stay separate from packed local member offsets and generic
object emission.

## In Scope

- Refresh focused prepared object-data, object emission, disassembly, and
  runtime evidence for static local storage rows.
- Identify whether the first owner is prepared object-data publication, static
  storage layout, initialization payload placement, relocation, or RV64
  object-data consumption.
- Repair one general static-storage object-data rule after proving storage
  identity, initialization state, and consumer point.

## Out Of Scope

- Packed local member offsets, byval call-boundary payloads, pointer-local
  lowering, callee-saved GPR preservation, CLI dump formatting, AArch64
  dispatch, or LLVM torture diagnosis.
- Generic RISC-V object-emission failures unless focused evidence proves the
  object-data storage route is first owner.
- Expectation rewrites, unsupported-marker changes, allowlists, timeout
  changes, runtime policy changes, or baseline acceptance changes.

## Acceptance Criteria

- Focused evidence names the first object-data/static-storage owner for both
  rows or records a justified split.
- The selected route proves storage identity and initialization payload from
  prepared facts through RV64 object emission or fails closed at the proven
  owner.
- The focused object-data static-storage subset passes or fails closed with
  precise diagnostics.
- Backend regression proof shows no new backend failures in the supervisor's
  chosen subset.

## Reviewer Reject Signals

- Reject merging packed member offsets or generic object emission into this
  static-storage route without proof.
- Reject named-case shortcuts for static local or initialized static local
  storage rows.
- Reject treating expectation updates, unsupported-marker downgrades,
  allowlist edits, helper renames, or classification-only edits as progress.
- Reject final assembly or object bytes as the only authority when prepared
  object-data facts are missing or ambiguous.
- Reject a patch that leaves the same static storage payload or relocation
  failure behind a new abstraction name.
