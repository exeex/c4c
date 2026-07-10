# RV64 Prepared Destination Publication

Status: Open
Type: Implementation
Parent: `ideas/open/658_backend_baseline_history_umbrella_triage.md`
Related:
- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
- `ideas/open/655_stack_destination_fan_in_authority_decomposition.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
Owning Layer: RV64 prepared destination and parameter-home
publication/consumption
Queue Order: 61
Proof Surface: current baseline rows 92, 103, 109, and 172 from
`log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`.

## Goal

Repair RV64 prepared destination publication and consumption for parameter
homes, scalar frame-slot destinations, call-result predicates, and
function-pointer return-chain destinations.

## Why This Exists

Step 2 grouped four current failed rows by labels naming prepared destination,
parameter-home publication, call-result predicate, or return-chain destination
behavior. These rows need a focused RV64 destination-publication route that
does not duplicate the prepared/prealloc stack fan-in authority work parked in
ideas 647 and 655.

## In Scope

- Refresh focused evidence for stack-passed parameter homes, scalar
  frame-slot destinations, prepared fused compare call-result predicates, and
  function-pointer return-chain destinations.
- Identify whether the first owner is prepared destination publication,
  parameter-home publication, destination-home consumption, or RV64 lowering
  of already-published prepared facts.
- Repair one general prepared destination/publication rule after proving
  source, destination, and consumer point.
- Preserve fail-closed diagnostics for missing or ambiguous destination facts.

## Out Of Scope

- Ordered final-state, mutual-exclusion, explicit-merge, or rejection
  authority for stack-destination fan-in from ideas 647 and 655.
- RV64 pointer-local postincrement, byval aggregate, object-data static
  storage, callee-saved GPR, packed member offset, AArch64, CLI, or LLVM
  torture work.
- Inferring destination authority from final assembly, diagnostics, filename,
  value id, source order, or testcase identity.
- Test expectation rewrites, unsupported-marker changes, allowlists, timeout
  changes, runtime policy changes, or baseline acceptance changes.

## Acceptance Criteria

- Focused evidence names one first owner for the four prepared
  destination/publication rows or records a smaller split with concrete proof.
- Published destination facts identify their owner and consumer point.
- The selected route passes the focused destination-publication subset or
  fails closed with precise diagnostics.
- Backend regression proof shows no new backend failures in the supervisor's
  chosen subset.

## Reviewer Reject Signals

- Reject turning this into another packet under ideas 647 or 655 without
  explicit lifecycle switch evidence.
- Reject RV64 consumer materialization before prepared facts publish the
  relevant destination or parameter home.
- Reject named-case fixes for parameter-home, frame-slot destination,
  call-result predicate, or return-chain tests.
- Reject expectation rewrites, unsupported-marker downgrades, allowlist edits,
  helper renames, or classification-only edits claimed as progress.
- Reject retaining the same missing or ambiguous destination-publication
  failure behind a renamed diagnostic.
