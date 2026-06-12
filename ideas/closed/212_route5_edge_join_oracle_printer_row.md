# 212 Route 5 edge/join oracle or printer row

## Goal

Replace exactly one named Route 5 edge/join helper-oracle row or prepared
printer row covering join transfer, parallel copy, move bundle, edge
publication, or source producer.

## Why This Exists

Phase D2 accepted this as one Step 4 proof-shape row. Row-level diagnostic
replacement is useful only if it preserves prepared edge/join oracle authority
and adjacent wrapper output.

## In Scope

- One named edge/join helper-oracle row or prepared printer row.
- Route-native evidence for that row.
- Prepared fallback for no-source, memory-source, unsupported move,
  duplicate/conflict, absent/mismatch, wrapper output stability, and
  branch/parallel-copy sanity.

## Out Of Scope

- Whole edge-publication, source-producer, move-bundle, joined-branch, wrapper,
  or printer-family migration.
- Branch/parallel-copy policy movement, public fallback removal, or prepared API
  deletion.

## Acceptance Criteria

- Positive: route-native evidence reproduces the named prepared success row.
- Absent: no edge/join fact reports the same no-source or absent status.
- Invalid: invalid source or publication reference preserves prepared status.
- Duplicate/conflict: duplicate or conflicting rows preserve prepared
  diagnostics.
- Mismatch: route/prepared disagreement preserves the prepared row.
- Fallback: unsupported move and branch/parallel-copy paths remain
  prepared-owned.
- Printer/debug, wrapper, and expected-string proof shows the named row and
  adjacent wrapper/joined-branch strings remain byte-stable.

## Reviewer Reject Signals

- Reject broad edge/join oracle, printer, wrapper, or joined-branch migration.
- Reject expected-string rewrites, helper renames, baseline refreshes, or
  unsupported downgrades.
- Reject testcase-shaped row matching that omits no-source, memory-source,
  unsupported move, conflict, mismatch, or fallback proof.
- Reject moving branch/parallel-copy policy into Route 5.
- Reject retaining the old diagnostic failure behind a route-native name.

## Closure Note

Closed after implementing the selected
`PreparedCurrentBlockJoinParallelCopySourceFact` helper-oracle row for the
named current-block join source success case. Route 5 evidence is attached as
diagnostic metadata only after agreement with prepared edge/join semantics,
while absent, invalid, duplicate/conflicting, memory-source, mismatch,
unsupported, branch/parallel-copy, printer, and wrapper paths retain prepared
or target-owned behavior.

Close proof used matching seven-test `test_before.log` and `test_after.log`
scope:
`backend_prepared_lookup_helper`, `backend_prepared_printer`,
`backend_prealloc_block_entry_publications`,
`backend_aarch64_current_block_join_routing`,
`backend_prepare_authoritative_join_ownership`,
`backend_x86_prepared_handoff_label_authority`, and
`backend_riscv_prepared_edge_publication`. The close-time regression guard
passed with 7/7 before and 7/7 after, with no new failures.
