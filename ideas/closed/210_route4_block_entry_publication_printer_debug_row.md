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

## Closure Note

Closed after replacing the selected available-register
`block_entry_publication` prepared value-location printer row with
route-native Route 4 attribution gated by prepared agreement and fail-closed
fallback. The accepted proof covered positive agreement, absent route evidence,
wrong references, mismatch, duplicate-reference fallback, byte-stable prepared
row text, and x86/riscv wrapper no-change checks.

Close proof:
`cmake --build --preset default --target backend_prealloc_block_entry_publications_test backend_prepared_printer_test backend_x86_publication_plan_reuse_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_block_entry_publications|backend_prepared_printer|backend_x86_publication_plan_reuse|backend_riscv_prepared_edge_publication)$' > test_after.log`

Close-time regression guard passed against matching four-test
`test_before.log` / `test_after.log` logs.
