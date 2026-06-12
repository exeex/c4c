# 208 Route 3 memory/source oracle or printer row

## Goal

Replace exactly one named Route 3 memory/source helper-oracle status row or one
prepared addressing printer row with route-native evidence while preserving
prepared diagnostic authority.

## Why This Exists

Phase D2 accepted this as one Step 4 proof-shape row. A row-wide replacement
can be useful only if it reproduces current prepared positive, negative, and
fallback behavior without changing strings.

## In Scope

- One named memory/source helper-oracle status row or one prepared addressing
  printer row.
- Route-native evidence for that row.
- Prepared fallback for absent, non-memory, ambiguity, mismatch, fallback, and
  target-policy-sensitive output.

## Out Of Scope

- Whole memory oracle families, printer sections, target-wrapper migration, or
  expected-string refreshes.
- Public fallback removal, prepared API deletion, aggregate migration, or broad
  `PreparedFunctionLookups` retirement.

## Acceptance Criteria

- Positive: route-native row matches the current prepared success status.
- Absent: no Route 3 fact reports the same fail-closed status as prepared.
- Invalid: invalid reference reports the same diagnostic status.
- Duplicate/conflict: ambiguous or alias/address conflict reports the same
  status.
- Mismatch: route/prepared disagreement preserves prepared status.
- Fallback: target-addressing fallback remains visible.
- Printer/debug and expected-string proof shows byte-stable row text unless a
  separate semantic proof justifies a string change.

## Reviewer Reject Signals

- Reject broad helper-oracle or printer migration disguised as one row.
- Reject helper renames, expectation rewrites, baseline refreshes, or weakened
  statuses as capability progress.
- Reject testcase-shaped status matching that does not preserve absent,
  invalid, ambiguity, mismatch, and fallback behavior.
- Reject target-addressing policy movement into Route 3.
- Reject hiding the old prepared diagnostic failure mode behind new route
  terminology.
