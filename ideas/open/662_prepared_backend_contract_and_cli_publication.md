# Prepared Backend Contract And CLI Publication

Status: Open
Type: Implementation
Parent: `ideas/open/658_backend_baseline_history_umbrella_triage.md`
Related:
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
Owning Layer: prepared backend contract publication and CLI dump exposure
Queue Order: 62
Proof Surface: current baseline rows 297, 299, 300, 301, 314, and 318 from
`log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`.

## Goal

Repair internal prepared backend contract publication and CLI exposure for
liveness, frame/stack call contracts, prepared printing, prealloc inline asm,
and prepared-BIR dump contract rows.

## Why This Exists

Step 2 classified several internal rows separately from RV64 runtime
lowering. They are contract-publication and CLI exposure failures, not target
runtime symptoms, and should be handled before target-specific AArch64 or
RISC-V object-emission rows are mixed into the same route.

## In Scope

- Refresh evidence for prepared liveness, prepared frame/stack call contract,
  prepared printer, prealloc inline asm, and CLI prepared-BIR dump rows.
- Identify whether the first owner is prepared contract production, printer
  formatting over an existing contract, CLI section exposure, or prealloc
  inline-asm contract publication.
- Repair one general contract-publication or CLI exposure rule with focused
  positive and negative evidence.
- Keep row 322's AArch64-specific publication and row 284's instruction
  dispatch under the separate AArch64 idea unless proof shows this contract
  route owns them first.

## Out Of Scope

- RV64 runtime lowering for pointer-local, byval, object-data, callee-saved
  GPR, packed member, or destination-publication rows.
- RISC-V object emission and AArch64 instruction dispatch implementation.
- LLVM torture owner discovery.
- Expectation rewrites, unsupported-marker changes, allowlist edits, timeout
  changes, runtime policy changes, or baseline acceptance changes.

## Acceptance Criteria

- Focused evidence names the first prepared contract or CLI exposure owner for
  the six rows, or records a smaller split with concrete proof.
- The selected repair exposes existing prepared facts faithfully or publishes
  missing contract facts with explicit ownership.
- The focused internal prepared/CLI subset passes or fails closed with precise
  diagnostics.
- Backend regression proof shows no new backend failures in the supervisor's
  chosen subset.

## Reviewer Reject Signals

- Reject treating CLI text-only expectation rewrites as capability progress.
- Reject merging target-specific AArch64 dispatch, RISC-V object emission, or
  RV64 runtime lowering into this contract-publication route without proof
  that prepared contract publication is first owner.
- Reject helper renames, diagnostic wording changes, classification-only
  edits, unsupported-marker downgrades, allowlist edits, or timeout changes
  claimed as progress.
- Reject named-case matching for prepared liveness, frame/stack call
  contracts, prepared printer, inline asm, or CLI dump rows.
- Reject leaving the same missing contract section or stale prepared-BIR
  exposure behind a new abstraction name.
