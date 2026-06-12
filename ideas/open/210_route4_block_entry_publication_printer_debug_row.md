# 210 Route 4 block-entry publication printer/debug row

## Goal

Replace exactly one block-entry publication printer/debug row from prepared
value-location or publication output with route-native evidence while keeping
the row text byte-stable.

## Why This Exists

Phase D2 narrowed the conditional Route 4 diagnostic candidate to this one Step
4 proof-shape row. It is not a wrapper-family, CLI dump, or publication
mechanics migration.

## In Scope

- One named block-entry publication printer/debug row.
- Route-native evidence that reproduces the current prepared semantics for that
  row.
- Prepared fallback for missing, wrong-reference, duplicate, ambiguous, and
  mismatched publication facts.
- No-change checks for x86/riscv wrapper output.

## Out Of Scope

- Wrapper-family migration, CLI dump section migration, broad printer/debug
  contraction, emitted-string redesign, or target policy movement.
- Public fallback removal, prepared API deletion, or aggregate migration.

## Acceptance Criteria

- Positive: the named publication row is reproduced from route-native evidence
  with the same prepared semantics.
- Absent: missing Route 4 publication keeps the current prepared row or absence
  status.
- Invalid: wrong-reference publication keeps current diagnostic behavior.
- Duplicate/conflict: duplicate or ambiguous rows keep current prepared
  diagnostics.
- Mismatch: route/prepared disagreement refuses route-native replacement.
- Fallback: prepared printer/debug remains authoritative.
- Printer/debug and expected-string proof shows exact byte-stable row text.
- Wrapper proof checks x86/riscv output for no change without migrating
  wrappers.

## Reviewer Reject Signals

- Reject wrapper-family, CLI dump, or broad printer/debug migration.
- Reject row text rewrites, helper renames, baseline refreshes, unsupported
  downgrades, or expected-string weakening.
- Reject testcase-shaped row emission that bypasses duplicate, ambiguity,
  invalid, mismatch, and fallback behavior.
- Reject moving publication mechanics or target output policy into Route 4.
- Reject retaining the same prepared-only diagnostic limitation under a new
  route-native label.
