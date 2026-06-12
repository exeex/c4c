# 211 Route 5 current-block join-source semantic reader

## Goal

Move exactly one current-block join-source semantic reader to Route 5
route/prepared agreement while retaining prepared edge, join, value-home, and
move-bundle authority.

## Why This Exists

Phase D2 accepted this as one Step 4 proof-shape row. Route 5 validates a
selected join-source identity, but prepared policy still owns movement and
emitted output.

## In Scope

- One named current-block join-source semantic reader.
- Route/prepared agreement for the same current-block join source.
- Prepared fallback for no-source, invalid, duplicate/conflict, and mismatched
  Route 5 facts.
- Proof that joined-branch and prepared output strings remain unchanged.

## Out Of Scope

- Parallel-copy scheduling, source/destination homes, execution-site placement,
  cycle temporaries, branch policy, final move records, emitted output, or
  wrapper migration.
- Whole edge-publication, source-producer, move-bundle, or x86 joined-branch
  migration.

## Acceptance Criteria

- Positive: route/prepared agreement provides the same current-block join
  source.
- Absent: no Route 5 fact keeps prepared no-source behavior.
- Invalid: invalid source reference fails closed to prepared behavior.
- Duplicate/conflict: duplicate or conflicting join-source rows keep prepared
  behavior.
- Mismatch: route/prepared disagreement keeps the prepared source.
- Fallback: move-bundle, branch, and parallel-copy policy remain
  prepared/target-owned.
- Wrapper and expected-string proof shows joined-branch and prepared output
  strings remain unchanged.

## Reviewer Reject Signals

- Reject broad edge-publication, source-producer, move-bundle, or x86
  joined-branch migration.
- Reject movement of branch, parallel-copy, execution-site, or final output
  policy into Route 5.
- Reject expectation rewrites, baseline refreshes, helper renames, or weaker
  supported-path contracts.
- Reject testcase-shaped join-source handling without general fail-closed
  route/prepared agreement for the named reader.
- Reject old prepared-only failure behavior hidden behind a new route API.

## Closure Note

Closed after migrating exactly one selected reader,
`current_block_join_prepared_query_source(...)`, to Route 5 route/prepared
agreement while retaining prepared fallback for absent, invalid, duplicate, and
mismatched route facts.

The accepted implementation preserved prepared and target ownership for join,
edge, move-bundle, branch, parallel-copy, output, and wrapper behavior. The
acceptance review found no implementation drift, scope creep, or
testcase-overfit.

Close proof used matching seven-test before/after logs for:
`backend_aarch64_current_block_join_routing`,
`backend_aarch64_instruction_dispatch`, `backend_prepared_lookup_helper`,
`backend_prepared_printer`, `backend_riscv_prepared_edge_publication`,
`backend_prepare_authoritative_join_ownership`, and
`backend_x86_prepared_handoff_label_authority`. The close-time regression guard
passed with 7/7 before and 7/7 after, with no new failures.
