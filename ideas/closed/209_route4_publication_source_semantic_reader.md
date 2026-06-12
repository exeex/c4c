# 209 Route 4 publication-source semantic reader

## Goal

Move exactly one publication-source semantic reader beside the existing
current-block publication reader to selected Route 4 identity with prepared
publication fallback retained.

## Why This Exists

Phase D2 accepted this as one Step 4 proof-shape row. Route 4 can identify a
selected publication source, but prepared publication mechanics and output
policy remain authoritative.

## In Scope

- One named publication-source semantic reader.
- Route/prepared agreement for the publication source identity.
- Prepared fallback for missing, duplicate, ambiguous, wrong-reference, and
  mismatched publication facts.
- Proof that x86/riscv wrapper output and prepared debug strings do not change.

## Out Of Scope

- Publication mechanics, move/home/storage policy, immediate payload spelling,
  wrapper compatibility, emitted/debug output, or target policy.
- Wrapper-family migration, CLI dump migration, public fallback removal, or
  prepared API deletion.

## Acceptance Criteria

- Positive: route/prepared agreement supplies the same publication source
  identity to the named reader.
- Absent: missing Route 4 fact uses prepared publication fallback.
- Invalid: wrong-reference publication fails closed.
- Duplicate/conflict: ambiguous publication rows keep prepared behavior.
- Mismatch: route/prepared disagreement keeps the prepared source.
- Fallback: publication mechanics and immediate/output policy remain
  prepared/target-owned.
- Wrapper and expected-string proof shows no output changes.

## Reviewer Reject Signals

- Reject broad publication or wrapper migration.
- Reject moving publication mechanics, immediate spelling, or target output
  policy into Route 4.
- Reject expectation rewrites, helper renames, unsupported downgrades, or
  baseline refreshes as evidence.
- Reject testcase-shaped handling of one publication case without a general
  route/prepared agreement rule for the named reader.
- Reject retaining the old prepared-only behavior behind a route-named wrapper.

## Closure Summary

Closed after the indirect-callee publication-source reader was migrated to the
Route 4 route/prepared agreement gate while retaining prepared fallback for
invalid, ambiguous, absent, and mismatched publication facts. Output stability
and wrapper/prepared-surface non-goals remained unchanged.

Accepted proof: `backend_aarch64_instruction_dispatch` and
`backend_prepared_lookup_helper` passed 2/2 after Step 4, and the close-time
regression guard compared matching 2/2 before/after logs with no new failures.
