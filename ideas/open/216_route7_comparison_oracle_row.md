# 216 Route 7 fused-compare or materialized-condition oracle row

## Goal

Replace exactly one Route 7 fused-compare or materialized-condition
helper-oracle row with route-native evidence while preserving branch-policy and
expected-string authority.

## Why This Exists

Phase D2 accepted this as one Step 4 proof-shape row. Row-level diagnostic
replacement can be useful only if it preserves current prepared fail-closed,
fallback, and string behavior.

## In Scope

- One named fused-compare or materialized-condition helper-oracle row.
- Route-native evidence for the named row.
- Prepared fallback for positive, absent-route, invalid-reference, duplicate,
  mismatch, unfused fallback, and prepared fallback behavior.
- Expected-string proof for helper-oracle and branch-control output.

## Out Of Scope

- Branch-policy cleanup, printer/debug contraction, wrapper contraction, final
  assembler changes, public fallback removal, or expected-string weakening.
- Route 8 return-chain work or generic route-index facade work.

## Acceptance Criteria

- Positive: route-native helper row matches the current fused or
  materialized-condition success row.
- Absent: absent Route 7 fact reports the same fail-closed status.
- Invalid: invalid reference reports the same status.
- Duplicate/conflict: duplicate or conflicting provenance rows preserve current
  diagnostic status.
- Mismatch: route/prepared disagreement preserves prepared oracle behavior.
- Fallback: unfused and prepared fallback behavior stays intact.
- Printer/debug and expected-string proof shows helper-oracle and branch-control
  strings remain byte-stable.
- Wrapper proof confirms no wrapper contraction is implied.

## Reviewer Reject Signals

- Reject branch-policy cleanup, printer/debug contraction, wrapper contraction,
  Route 8 work, or generic route-index migration.
- Reject expected-string rewrites, helper renames, baseline refreshes,
  unsupported downgrades, or weaker oracle contracts.
- Reject testcase-shaped oracle handling without absent, invalid, conflict,
  mismatch, unfused fallback, and prepared fallback proof.
- Reject moving branch policy or final assembler behavior into Route 7.
- Reject hiding the existing prepared oracle failure mode behind a route-native
  label.
