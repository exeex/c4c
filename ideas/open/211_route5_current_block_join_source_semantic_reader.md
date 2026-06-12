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
